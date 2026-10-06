// NOLINTBEGIN(performance-avoid-endl)
#include<simpleBlockSdk.hpp>
int main(){
    PubSubSdk::AppPublisher publisher;
    PubSubSdk::Status status=publisher.connect();
    std::cout << "1.add Topic\n2.remove Topic\n3.publish message\n4.exit"<< std::endl;
    try{

        while(true){
            int n;
            std::cin >> n;
            if(n==1){
                std::string tpc;
                std::cout<< "write Topic Name to Add:"<< std::endl;
                std::cin>> tpc;
                publisher.addTpc(tpc);
                std::cout << "trying to add Tpc:"<< tpc<< std::endl;
            }else if(n==2){
                std::string tpc;
                std::cout<< "write Topic Name to Rem:"<< std::endl;
                std::cin>> tpc;
                publisher.remTpc(tpc);
            }else if(n==3){
                std::string tpc;
                std::cout<< "write Topic Name for publish:"<< std::endl;
                std::cin>> tpc;
                std::string msg;
                std::cout<< "write messageto publish:"<< std::endl;
                std::cin>> msg;
                publisher.publish(tpc,msg);
            }else{
                exit(0);
            }
            std::cout << publisher.checkStatus()<< std::endl;
        }
    }catch(std::runtime_error& e){
        std::cout << e.what()<< std::endl;
    }
}
// NOLINTEND(performance-avoid-endl)
