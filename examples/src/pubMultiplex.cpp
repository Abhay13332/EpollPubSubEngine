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

auto getRandomMsg(){
        std::vector<std::pair<std::string, std::string>> msgs = {
        //NOTE: this example texts is ai generated;
    {"general", "Good morning everyone! Hope you all have a productive day."},
    {"general", "Did anyone catch the game last night? What a crazy finish!"},
    {"general", "Is the office coffee machine broken again or is it just me?"},
    {"general", "Don't forget, we have the team lunch at 12:30 PM today."},
    {"general", "Has anyone tried that new burger place down the street?"},
    {"general", "Quick reminder to submit your weekly timesheets before Friday."},
    {"general", "Happy Friday! Any exciting weekend plans?"},

    {"tech", "Has anyone experimented with C++23 features in production yet?"},
    {"tech", "I'm hitting a strange linker error with the new third-party library."},
    {"tech", "Tabs vs Spaces debate is officially banned in this channel. Use whatever formatting tool dictates."},
    {"tech", "Can someone review my pull request? It's a quick fix for the memory leak."},
    {"tech", "The database migration script is ready for staging deployment."},
    {"tech", "Pro tip: Always check your loop conditions before pushing code to the main branch."},
    {"tech", "Is it worth migrating our legacy backend from REST to gRPC?"},

    {"system.alerts", "WARNING: CPU usage on server-04 has exceeded 90% for over 5 minutes."},
    {"system.alerts", "CRITICAL: Database replica-2 has disconnected from the primary node!"},
    {"system.alerts", "INFO: Scheduled backup completed successfully. Total size: 45.2 GB."},
    {"system.alerts", "INFO: Deployment of microservice-v2.1.4 successfully rolled out."},

    {"random", "There are 10 types of people in the world: those who understand binary, and those who don't."},
    {"random", "If a tree falls in a forest and no one is around to hear it, does it still generate a log event?"},
    {"random", "Currently fueling my debugging session with a concerning amount of caffeine."},
    {"random", "My code doesn't work and I don't know why... Oh wait, it works now and I still don't know why."},

    {"general", ""}, 
    {"", "Message sent to an empty topic string test."}, 
    {"tech", "Special characters test: !@#$%^&*()_+=-{}[]|\\:;\"'<>,.?/~`"}, 
    {"random", "UTF-8 support verification:  🌵 漢字 Русский string test."} 
};
  return msgs[randomIdxgen(msgs.size())];
}
std::string randomTopicGen(){
    std::vector tpcs={"general","system.alerts","tech","random"};
    return tpcs[randomIdxgen(tpcs.size())];
}
void pub(){
    PubSubSdk::AppPublisher pub;
    pub.connect();
    std::set<std::string> pubTpc;
    while(true){
        auto [tpc,msg]=getRandomMsg();
        if(!pubTpc.contains(tpc)){
            pub.addTpc(tpc);
            debug::print("addtpc ",tpc," status:",pub.checkStatus());
            pubTpc.insert(tpc);
        }
        pub.publish(tpc,msg);
        debug::print("publish on tpc ",tpc," status:",pub.checkStatus());

        sleep(1);
        if(randomProbGen(0.3)){
            auto it = randomTopicGen();
            pub.remTpc(it);
            pubTpc.erase(it);
            debug::print("remove  tpc ",tpc," status:",pub.checkStatus());
        }
    }
}


int main(){
  pub();
}
// NOLINTEND(performance-avoid-endl)
