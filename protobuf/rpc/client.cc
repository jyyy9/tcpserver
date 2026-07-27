#include "rpc.pb.h"
#include "rpc.grpc.pb.h"
#include <grpcpp/grpcpp.h>

int main()
{
    //初始化grpc服务
    grpc_init();
    // 实例化请求对象，设置请求信息
    calculate::AddRequest request;
    request.set_num1(111);
    request.set_num2(222);
    // 实例化响应对象，用于获取响应结果
    calculate::AddResponse response;
    // 实例化一个 grpc::Channel对象（这就是一个通信客户端）
    auto channel = grpc::CreateChannel("127.0.0.1:9000", 
        grpc::InsecureChannelCredentials());
    // 实例化一个 class Stub 对象，发起指定的RPC调用
    calculate::Calculate::Stub stub(channel);
    ::grpc::ClientContext context;
    ::grpc::Status isOk = stub.Add(&context, request, &response);
    if (isOk.ok()) {
        std::cout << "响应结果:" << response.result() << std::endl;
    }else {
        std::cout << isOk.error_message() << std::endl;
    }
    //销毁grpc服务
    grpc_shutdown();
    return 0;
}