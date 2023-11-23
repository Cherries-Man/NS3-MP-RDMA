
#include "mp-rdma-hw.h"

namespace ns3
{
    // 注册类型
    TypeId MpRdmaHw::GetTypeId(void)
    {
        static TypeId tid = TypeId("ns3::MpRdmaHw")
                                .SetParent<RdmaHw>()
                                .AddConstructor<MpRdmaHw>();
        return tid;
    }

    // 构造函数
    MpRdmaHw::MpRdmaHw()
    {
        // 初始化成员变量
    }

    // 其他辅助函数的实现...

} /* namespace ns3 */
