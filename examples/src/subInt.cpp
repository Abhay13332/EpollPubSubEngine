// NOLINTBEGIN(performance-avoid-endl)
#include<simpleBlockSdk.hpp>
int main(){
    PubSubSdk::AppSubsCriber sub;
    sub.connect();
    sub.runBgThread();
    std::cout << "1.add Topic\n2.remove Topic\n3.exit"<< std::endl;
    while(true){
        int n;
        std::cin >> n;
        if(n==1){
            std::string tpc;
            std::cout<< "write Topic Name to Add:"<< std::endl;
            std::cin>> tpc;
            // NOLINTNEXTLINE(performance-unnecessary-value-param)
            sub.addTopic(tpc,[](const std::string msg){
                std::cout << "get msg:"+msg << std::endl;
            });
        }else if(n==2){
            std::string tpc;
            std::cout<< "write Topic Name to Rem:"<< std::endl;
            std::cin>> tpc;
            sub.removeTopic(tpc);
        }else{
            exit(0);
        }
    }
}
// NOLINTEND(performance-avoid-endl)