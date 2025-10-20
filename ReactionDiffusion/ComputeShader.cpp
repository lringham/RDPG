#include "ComputeShader.h"
#include "Utils.h"
#include "ResourceLocations.h"

#include <random>


ComputeShader::ComputeShader(const std::map<std::string, std::string>& includes, const std::string& shaderFilename)
{
    addShaderStage(GL_COMPUTE_SHADER, loadTextFile(ResourceLocations::instance().shadersPath() + shaderFilename), includes);
    createShaderProgram();
}

ComputeShader::~ComputeShader()
{}

// Max number of work groups that may be dispatched to a compute shader.
void ComputeShader::maxGlobalWorkgroupSize(int& x, int& y, int& z)
{
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &z);
}

// Max size of a work groups that may be used during compilation of a compute shader.
void ComputeShader::maxLocalWorkgroupSize(int& x, int& y, int& z)
{
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &x);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &y);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &z);
}

// Max number of invocations in a single local work group (the product of the
// three dimensions) that may be dispatched to a compute shader
int ComputeShader::maxLocalInvocations()
{
    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    return work_grp_inv;
}

void ComputeShader::printComputeInfo()
{
    //Determine the maximum global workgroup size in all 3 dimension
    int work_grp_cnt[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_grp_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_grp_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_grp_cnt[2]);
    printf("Max number of workgroups that may be dispatched to a compute-shader (Invocation Space)\n\tx:%i y:%i z:%i\n",
        work_grp_cnt[0], work_grp_cnt[1], work_grp_cnt[2]);

    //Determine the maximum local workgroup size (Defined in shader with a layout)
    int work_grp_size[3];
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_grp_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_grp_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_grp_size[2]);
    printf("Max size of a workgroup that may be used during compilation of a compute-shader\n\tx:%i y:%i z:%i\n",
        work_grp_size[0], work_grp_size[1], work_grp_size[2]);

    //Determine the maximum number of work group units that a local work group in the compute shader is allowed
    int work_grp_inv;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_grp_inv);
    printf("Max local work group invocations\n\t%i\n", work_grp_inv);
}

void ComputeShader::dispatch(GLuint workGroupsX, GLuint workGroupsY, GLuint workGroupsZ)
{
    glDispatchCompute(workGroupsX, workGroupsY, workGroupsZ);
}

void ComputeShader::wait(GLuint barrierType)
{
    glFlush();
    glMemoryBarrier(barrierType);
}
