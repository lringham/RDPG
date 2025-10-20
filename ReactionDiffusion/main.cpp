#include "RDPG.h"


int main(int argc, char** argv)
{
    RDPG rdpg;
    rdpg.init(argc, argv);
    return rdpg.run();
}