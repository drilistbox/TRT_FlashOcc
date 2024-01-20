/*
 * Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <NvInfer.h>
#include <cmath>
#include <string>
#include <vector>
#include <unistd.h>
#include <iostream>


namespace bevdet                                     // ��Ӧ��torch��ע��onnx ����ʱ�޶�������
{
static const char *PRE_PLUGIN_NAME {"Preprocess"};   // ��Ӧ��torch��ע��onnx ����ʱ�޶�����������
static const char *PRE_PLUGIN_VERSION {"1"};         // �汾�� ���޶�
} // namespace

namespace nvinfer1
{
    /* 
* ����������Ҫ����������, һ������ͨ��Plugin��, һ����PluginCreator��
*  - Plugin���ǲ���࣬����д����ľ���ʵ��
*  - PluginCreator���ǲ�������࣬�����������󴴽���������ò���Ǵ������ߵ�
*/
class PreprocessPlugin : public IPluginV2DynamicExt
{
private:
    const std::string mName;
    std::string       mNamespace;
    struct
    {
        int crop_h;
        int crop_w;
        float resize_radio;
    } mParams; // ��������op��Ҫ�в�����ʱ�򣬰���Щ��������Ϊ��Ա���������Ե����ó������壬Ҳ���������������һ���ṹ��

public:
    /*
     * �����ڱ���Ĺ����л��д�������δ������ʵ���Ĺ���
     * 1. parse�׶�: ��һ�ζ�ȡonnx��parse�����������ȡ������Ϣ��ת��ΪTensorRT��ʽ
     * 2. clone�׶�: parse�����Ժ�TensorRTΪ��ȥ�Ż��������Ḵ�ƺܶั�����������кܶ��Ż����ԡ�Ҳ�����������ʱ�򹩲�ͬ��context���������ʱ��ʹ��
     * 3. deseriaze�׶�: �����л��õ�Plugin���з����л���ʱ��Ҳ��Ҫ���������ʵ��
    */
    PreprocessPlugin() = delete;                                                             //Ĭ�Ϲ��캯����һ��ֱ��delete
    PreprocessPlugin(const std::string &name, int crop_h, int crop_w, float resize_radio);   //parse, cloneʱ���õĹ��캯��
    PreprocessPlugin(const std::string &name, const void *buffer, size_t length);            //�����л���ʱ���õĹ��캯��
    ~PreprocessPlugin();

    // Method inherited from IPluginV2
    const char *getPluginType() const noexcept override;
    const char *getPluginVersion() const noexcept override;
    int32_t     getNbOutputs() const noexcept override;
    int32_t     initialize() noexcept override;
    void        terminate() noexcept override;
    size_t      getSerializationSize() const noexcept override;
    void        serialize(void *buffer) const noexcept override;
    void        destroy() noexcept override;
    void        setPluginNamespace(const char *pluginNamespace) noexcept override;
    const char *getPluginNamespace() const noexcept override;

    // Method inherited from IPluginV2Ext
    DataType getOutputDataType(int32_t index, DataType const *inputTypes, int32_t nbInputs) const noexcept override;
    void     attachToContext(cudnnContext *contextCudnn, cublasContext *contextCublas, IGpuAllocator *gpuAllocator) noexcept override;
    void     detachFromContext() noexcept override;

    // Method inherited from IPluginV2DynamicExt
    IPluginV2DynamicExt *clone() const noexcept override;
    DimsExprs            getOutputDimensions(int32_t outputIndex, const DimsExprs *inputs, int32_t nbInputs, IExprBuilder &exprBuilder) noexcept override;
    bool                 supportsFormatCombination(int32_t pos, const PluginTensorDesc *inOut, int32_t nbInputs, int32_t nbOutputs) noexcept override;
    void                 configurePlugin(const DynamicPluginTensorDesc *in, int32_t nbInputs, const DynamicPluginTensorDesc *out, int32_t nbOutputs) noexcept override;
    size_t               getWorkspaceSize(const PluginTensorDesc *inputs, int32_t nbInputs, const PluginTensorDesc *outputs, int32_t nbOutputs) const noexcept override;
    int32_t              enqueue(const PluginTensorDesc *inputDesc, const PluginTensorDesc *outputDesc, const void *const *inputs, void *const *outputs, void *workspace, cudaStream_t stream) noexcept override;

protected:
    // To prevent compiler warnings
    using nvinfer1::IPluginV2::enqueue;
    using nvinfer1::IPluginV2::getOutputDimensions;
    using nvinfer1::IPluginV2::getWorkspaceSize;
    using nvinfer1::IPluginV2Ext::configurePlugin;
};

class PreprocessPluginCreator : public IPluginCreator
{
private:
    static PluginFieldCollection    mFC;           //����plugionFields��������Ȩ�غͲ�����������Ϣ���ݸ�Plugin���ڲ�ͨ��createPlugin��������������plugin
    static std::vector<PluginField> mAttrs;        //��������������op����Ҫ��Ȩ�غͲ���, ��onnx�л�ȡ, ͬ����parse��ʱ��ʹ��
    std::string                     mNamespace;

public:
    PreprocessPluginCreator();  //��ʼ��mFC�Լ�mAttrs
    ~PreprocessPluginCreator();
    const char*                  getPluginName() const noexcept override;
    const char*                  getPluginVersion() const noexcept override;
    const PluginFieldCollection* getFieldNames() noexcept override;
    const char*                  getPluginNamespace() const noexcept override;
    IPluginV2DynamicExt *        createPlugin(const char* name, const PluginFieldCollection* fc) noexcept override;  //ͨ������������mFC������Plugin�����������Plugin�Ĺ��캯��
    IPluginV2DynamicExt *        deserializePlugin(const char* name, const void* serialData, size_t serialLength) noexcept override;
    void                         setPluginNamespace(const char* pluginNamespace) noexcept override;
};

} // namespace nvinfer1
