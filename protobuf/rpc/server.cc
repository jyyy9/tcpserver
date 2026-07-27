#include <grpcpp/grpcpp.h>
#include "rpc.grpc.pb.h"
#include "rpc.pb.h"

// class CalculateImpl : public calculate::Calculate::Service {
class CalculateImpl : public calculate::Calculate {
    public:
        // ::grpc::Status Add(::grpc::ServerContext* context, 
        //     const ::calculate::AddRequest* request, 
        //     ::calculate::AddResponse* response) {
        //     //针对这个虚函数进行重写，实现真正的数据处理逻辑
        //     int result = request->num1() + request->num2();
        //     response->set_result(result);
        //     // done->Run();  //告诉rpc服务器：当前请求处理完毕，可以响应结果了
        //     return ::grpc::Status::OK;
        // }
};

int main()
{
    //初始化grpc服务
    grpc_init();
    //实例化grpc服务器建造者对象
    grpc::ServerBuilder builder;
    //设置监听信息
    // grpc::InsecureServerCredentials() 使用非加密通信
    builder.AddListeningPort("0.0.0.0:9000", grpc::InsecureServerCredentials());
    //注册计算服务，
    CalculateImpl impl;
    builder.RegisterService(&impl);
    //构建并启动服务器
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    //销毁grpc服务
    grpc_shutdown();
    return 0;
}