#include<simpleBlockSdk.hpp>
#include<random>
#include<set>
thread_local std::random_device rd;  
thread_local std::mt19937 gen(rd()); 
// NOLINTBEGIN(performance-avoid-endl)

auto randomIdxgen(int size){
    std::uniform_int_distribution<size_t> distrib(0, size - 1);
    return distrib(gen);
}
int randomProbGen(double probOfOne) {
    std::bernoulli_distribution distrib(probOfOne);
    return distrib(gen); 
}

// NOLINTNEXTLINE(performance-unnecessary-value-param)
void pr(const std::string msg){
    std::cout << msg << std::endl;
}

std::string randomTopicGen(){
    std::vector tpcs={"general","system.alerts","tech","random"};
    return tpcs[randomIdxgen(tpcs.size())];
}
void sub(){
    PubSubSdk::AppSubsCriber sub;
    sub.connect();
    sub.runBgThread();
    std::set<std::string> subTpc;
    sub.addTopic("general",pr);
    while(true){
        if(randomProbGen(0.6)){
            auto tpc=randomTopicGen();
            subTpc.insert(tpc);
            sub.addTopic(tpc, pr);
            std::cout <<"add  tpc "<<tpc<< std::endl;;
        }

        sleep(3);
         if(randomProbGen(0.6)){
            auto tpc=randomTopicGen();
            subTpc.erase(tpc);
            sub.removeTopic(tpc);
            std::cout <<"remove  tpc "<<tpc<< std::endl;

        }
    }
}
// NOLINTEND(performance-avoid-endl)
int main(){
    sub();
}