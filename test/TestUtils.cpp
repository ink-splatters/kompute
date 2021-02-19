
#include <iostream>
#include <vector>

#include <shaderc/shaderc.hpp>

#include <glslang/Public/ShaderLang.h>
#include <StandAlone/ResourceLimits.h>
#include <SPIRV/GlslangToSpv.h>

static std::vector<char> spirv_from_string(const std::string& source,
                                   const std::string& entryPoint = "main",
                                   shaderc_optimization_level optimization = shaderc_optimization_level_zero,
                                   std::vector<std::pair<std::string,std::string>> definitions = {}) {

        // Initialize glslang library.
    glslang::InitializeProcess();

    const EShLanguage language = EShLangCompute;
    glslang::TShader shader(language);

    const char *file_name_list[1] = {""};
    const char *shader_source     = reinterpret_cast<const char *>(source.data());
    shader.setStringsWithLengthsAndNames(&shader_source, nullptr, file_name_list, 1);
    shader.setEntryPoint(entryPoint.c_str());
    shader.setSourceEntryPoint(entryPoint.c_str());

    std::string info_log = "";
    const EShMessages messages = static_cast<EShMessages>(EShMsgDefault | EShMsgVulkanRules | EShMsgSpvRules);
    if (!shader.parse(&glslang::DefaultTBuiltInResource, 100, false, messages))
    {
        info_log = std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog());
        throw std::runtime_error(info_log);
    }


    // Add shader to new program object.
    glslang::TProgram program;
    program.addShader(&shader);
    // Link program.
    if (!program.link(messages))
    {
        info_log = std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
        throw std::runtime_error(info_log);
    }

    // Save any info log that was generated.
    if (shader.getInfoLog())
    {
        info_log += std::string(shader.getInfoLog()) + "\n" + std::string(shader.getInfoDebugLog()) + "\n";
    }

    if (program.getInfoLog())
    {
        info_log += std::string(program.getInfoLog()) + "\n" + std::string(program.getInfoDebugLog());
    }

    glslang::TIntermediate *intermediate = program.getIntermediate(language);
    // Translate to SPIRV.
    if (!intermediate)
    {
        info_log += "Failed to get shared intermediate code.\n";
        throw std::runtime_error(info_log);
    }

    spv::SpvBuildLogger logger;
    std::vector<std::uint32_t> spirv;
    glslang::GlslangToSpv(*intermediate, spirv, &logger);
    info_log += logger.getAllMessages() + "\n";

    // Shutdown glslang library.
    glslang::FinalizeProcess();

    return std::vector<char>((char*)spirv.data(), (char*)(spirv.data()+spirv.size()) );
}
