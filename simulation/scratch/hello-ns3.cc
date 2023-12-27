#include <iostream>
// #include <ns3/core-module.h>
#include <ns3/myf-hello.h>

int main()
{
    std::cout << "Hello, NS-3!" << std::endl;
    // std::cout << "Simulator::Now() = " << ns3::Simulator::Now().GetTimeStep() << std::endl;
    MyfHello myfHello;
    myfHello.hello();
    return 0;
}
