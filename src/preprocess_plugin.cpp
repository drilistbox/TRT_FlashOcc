#include "preprocess_plugin.h"
#include <map>
#include <cstring>
#include "common.h"

// /* customPreprocessPlugin�ĺ˺����ӿڲ��� */
void customPreprocessPlugin(const int n_img, const int src_img_h, const int src_img_w, 
    const int dst_img_h, const int dst_img_w, const int type_int,
    const void *const *inputs, void *const *outputs, void *workspace, cudaStream_t stream,
    const float offset_h, const float offset_w, const float resize_radio);

namespace nvinfer1
{

/********************ע��PluginCreator*****************************/
REGISTER_TENSORRT_PLUGIN(PreprocessPluginCreator);


/*********************��̬����������*******************************/
// class PreprocessPluginCreator
PluginFieldCollection    PreprocessPluginCreator::mFC {};
std::vector<PluginField> PreprocessPluginCreator::mAttrs;


/*********************CustomScalarPluginʵ�ֲ���***********************/
// class PreprocessPlugin
PreprocessPlugin::PreprocessPlugin(const std::string &name, int crop_h, int crop_w, float resize_radio):
    mName(name){
    mParams.crop_h = crop_h;
    mParams.crop_w = crop_w;
    mParams.resize_radio = resize_radio;
}

PreprocessPlugin::PreprocessPlugin(const std::string &name, const void *buffer, size_t length):
    mName(name){
    memcpy(&mParams, buffer, sizeof(mParams));
}

PreprocessPlugin::~PreprocessPlugin(){
    /* �����������������Ҫ���κ����飬�������ڽ�����ʱ����Զ�����terminate��destroy */
}

IPluginV2DynamicExt *PreprocessPlugin::clone() const noexcept {
    auto p = new PreprocessPlugin(mName, &mParams, sizeof(mParams));
    p->setPluginNamespace(mNamespace.c_str());
    return p;
}

int32_t PreprocessPlugin::getNbOutputs() const noexcept {
    /* һ����˵���в����ʵ�ֲ��һ�� */
    return 1;
}
 
DataType PreprocessPlugin::getOutputDataType(int32_t index, DataType const *inputTypes, 
                                                                int32_t nbInputs) const noexcept {
    // return DataType::kHALF;
    return DataType::kFLOAT;
}

DimsExprs PreprocessPlugin::getOutputDimensions(int32_t outputIndex, const DimsExprs *inputs, 
                                        int32_t nbInputs, IExprBuilder &exprBuilder) noexcept {
    int input_h = inputs[0].d[2]->getConstantValue();
    int input_w = inputs[0].d[3]->getConstantValue(); // * 4;

    int output_h = input_h * mParams.resize_radio - mParams.crop_h;
    int output_w = input_w * mParams.resize_radio - mParams.crop_w;

    DimsExprs ret;
    ret.nbDims = inputs[0].nbDims;
    ret.d[0] = inputs[0].d[0];
    ret.d[1] = inputs[0].d[1];
    ret.d[2] =  exprBuilder.constant(output_h);
    ret.d[3] =  exprBuilder.constant(output_w);
    
    return ret; 
}

bool PreprocessPlugin::supportsFormatCombination(int32_t pos, const PluginTensorDesc *inOut,
                                                    int32_t nbInputs, int32_t nbOutputs) noexcept {
    bool res;
    switch (pos)
    {
    case 0: // images
        res = (inOut[0].type == DataType::kINT8 || inOut[0].type == DataType::kINT32) && 
                inOut[0].format == TensorFormat::kLINEAR;
        break;
    case 1: // mean
        res = (inOut[1].type == DataType::kFLOAT) &&
                inOut[1].format == TensorFormat::kLINEAR;
        break;
    case 2: // std
        res = (inOut[2].type == DataType::kFLOAT) &&
                inOut[2].format == TensorFormat::kLINEAR;
        break;
    case 3: // ��� img tensor
        res = (inOut[3].type == DataType::kFLOAT || inOut[3].type == DataType::kHALF) && 
                inOut[3].format == inOut[0].format;

        // res = inOut[3].type == DataType::kHALF && inOut[3].format == inOut[0].format;
        break;
    default: 
        res = false;
    }
    return res;
}

void PreprocessPlugin::configurePlugin(const DynamicPluginTensorDesc *in, int32_t nbInputs, 
                                    const DynamicPluginTensorDesc *out, int32_t nbOutputs) noexcept {
    return;
}

size_t PreprocessPlugin::getWorkspaceSize(const PluginTensorDesc *inputs, int32_t nbInputs, 
                                const PluginTensorDesc *outputs, int32_t nbOutputs) const noexcept {
    return 0;
}

int32_t PreprocessPlugin::enqueue(const PluginTensorDesc *inputDesc, const PluginTensorDesc *outputDesc,
    const void *const *inputs, void *const *outputs, void *workspace, cudaStream_t stream) noexcept {
    /*
     * Plugin�ĺ��ĵĵط���ÿ���������һ���Լ��Ķ��Ʒ���
     * Pluginֱ�ӵ���kernel�ĵط�
    */
    float offset_h = mParams.crop_h / mParams.resize_radio;
    float offset_w = mParams.crop_w / mParams.resize_radio;
    float resize_radio = mParams.resize_radio;

    int n_img = inputDesc[0].dims.d[0];
    int src_img_h = inputDesc[0].dims.d[2];
    int src_img_w = inputDesc[0].dims.d[3]; // * 4;
    
    int dst_img_h = outputDesc[0].dims.d[2];
    int dst_img_w = outputDesc[0].dims.d[3];
    int type_int = int(outputDesc[0].type);


    customPreprocessPlugin(n_img, src_img_h, src_img_w, 
    dst_img_h, dst_img_w, type_int,
    inputs, outputs, workspace, stream,
    offset_h, offset_w, resize_radio);
    return 0;
}

void PreprocessPlugin::destroy() noexcept {
    delete this;
    return;
}

int32_t PreprocessPlugin::initialize() noexcept {
    return 0;
}

void PreprocessPlugin::terminate() noexcept {
    return;
}

size_t PreprocessPlugin::getSerializationSize() const noexcept {
    /* ��������еĲ���������mParams�еĻ�, һ����˵���в����ʵ�ֲ��һ�� */
    return sizeof(mParams);
}

void PreprocessPlugin::serialize(void *buffer) const noexcept {
    memcpy(buffer, &mParams, sizeof(mParams));
    return;
}

void PreprocessPlugin::setPluginNamespace(const char *pluginNamespace) noexcept {
    mNamespace = pluginNamespace;
    return;
}

const char *PreprocessPlugin::getPluginNamespace() const noexcept {
    /* ���в��ʵ�ֶ���� */
    return mNamespace.c_str();
}

const char *PreprocessPlugin::getPluginType() const noexcept {
    /* һ����˵���в����ʵ�ֲ��һ�� */
    return bevdet::PRE_PLUGIN_NAME;
}

const char *PreprocessPlugin::getPluginVersion() const noexcept {
    return bevdet::PRE_PLUGIN_VERSION;
}

void PreprocessPlugin::attachToContext(cudnnContext *contextCudnn, cublasContext *contextCublas, 
                                                        IGpuAllocator *gpuAllocator) noexcept {
    return;
}

void PreprocessPlugin::detachFromContext() noexcept {
    return;
}

PreprocessPluginCreator::PreprocessPluginCreator() {
    
    mAttrs.clear();
    mAttrs.emplace_back(PluginField("crop_h", nullptr, PluginFieldType::kINT32, 1));
    mAttrs.emplace_back(PluginField("crop_w", nullptr, PluginFieldType::kINT32, 1));
    mAttrs.emplace_back(PluginField("resize_radio", nullptr, PluginFieldType::kFLOAT32, 1));

    mFC.nbFields = mAttrs.size();
    mFC.fields   = mAttrs.data();
}

PreprocessPluginCreator::~PreprocessPluginCreator() {
}


IPluginV2DynamicExt *PreprocessPluginCreator::createPlugin(const char *name, 
                                    const PluginFieldCollection *fc) noexcept {
    /*
     * ͨ��Creator����һ��Plugin��ʵ�֣����ʱ���ͨ��mFC��ȡ����Ҫ�Ĳ���, ��ʵ����һ��Plugin
     * ��������У�������scalar��scale������������fc��ȡ������Ӧ����������ʼ�����plugin
    */
    const PluginField *fields = fc->fields;

    int crop_h = -1;
    int crop_w = -1;
    float resize_radio = 0.f;

    for (int i = 0; i < fc->nbFields; ++i){
        if(std::string(fc->fields[i].name) == std::string("crop_h")){
            crop_h = *reinterpret_cast<const int *>(fc->fields[i].data);
        }
        else if(std::string(fc->fields[i].name) == std::string("crop_w")){
            crop_w = *reinterpret_cast<const int *>(fc->fields[i].data);
        }
        else if(std::string(fc->fields[i].name) == std::string("resize_radio")){
            resize_radio = *reinterpret_cast<const float *>(fc->fields[i].data);
        }
    }
    PreprocessPlugin *pObj = new PreprocessPlugin(name, crop_h, crop_w, resize_radio);
    pObj->setPluginNamespace(mNamespace.c_str());
    return pObj;
}

IPluginV2DynamicExt *PreprocessPluginCreator::deserializePlugin(const char *name, 
                                        const void *serialData, size_t serialLength) noexcept {
    /* �����л������ʵ����ʵ����һ����������в��ʵ�ֶ���� */
    PreprocessPlugin *pObj = new PreprocessPlugin(name, serialData, serialLength);
    pObj->setPluginNamespace(mNamespace.c_str());
    return pObj;
}

void PreprocessPluginCreator::setPluginNamespace(const char *pluginNamespace) noexcept {
    /* ���в��ʵ�ֶ���� */
    mNamespace = pluginNamespace;
    return;
}

const char *PreprocessPluginCreator::getPluginNamespace() const noexcept {
    return mNamespace.c_str();
}

const char *PreprocessPluginCreator::getPluginName() const noexcept {
    return bevdet::PRE_PLUGIN_NAME;
}

const char *PreprocessPluginCreator::getPluginVersion() const noexcept {
    /* һ����˵���в����ʵ�ֲ��һ�� */
    return bevdet::PRE_PLUGIN_VERSION;
}

const PluginFieldCollection *PreprocessPluginCreator::getFieldNames() noexcept {
    /* ���в��ʵ�ֶ���� */
    return &mFC;
}

} // namespace nvinfer1
