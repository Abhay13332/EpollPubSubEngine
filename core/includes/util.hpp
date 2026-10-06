#ifndef UTIL_HPP
#define UTIL_HPP
#include<exception>
#include <functional>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include <iomanip>
#include <fstream>
#ifndef NDEBUG
    // In Debug builds, execute the function or statement directly
    #define DEBUG_RUN(...) do { __VA_ARGS__ } while(0)
#else
    // In Release builds, replace it with nothing (completely removed)
    #define DEBUG_RUN(...) do { } while(0)
#endif

namespace debug {

    template <typename... Args>
    static void print( Args&&... args) {
        DEBUG_RUN(
        ((std::cout << args), ...);
        std::cout << std::endl;// NOLINT(performance-avoid-endl)
        );
    }
    
}
namespace Logging{
    class Logger {
        friend void setDefaultLogger(Logger&);
        friend Logger* getDefaultLogger();
        std::string logBuff;
        std::ofstream logFile;
        inline static Logger *defaultLogger=nullptr;
        public:
        Logger(std::string filepath){ //NOLINT(performance-unnecessary-value-param)
            logFile.open(filepath,std::ios::out| std::ios::app);
            if (!logFile.is_open()) {
                std::string backupPath = "backup_log_" + std::to_string(getpid()) + ".txt";
                logFile.open(backupPath,  std::ios::out | std::ios::app);
                if (!logFile.is_open()) {
                    throw std::runtime_error( "Error creating file.\n");
                }
            }
        }

        std::string getTimestamp() {
            auto now = std::chrono::system_clock::now();
            auto inTimeT = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&inTimeT), "%Y-%m-%d %X");
            return ss.str();
        }
        template <typename... Args>
        void updateBuffer(Args&&... args){
           std::stringstream ss;
            ((ss << std::forward<Args>(args) << " "), ...);
            logBuff.append(ss.str()).append("\n");

        }
        void tryDump(){
            if(logBuff.size()>4096 ){
               forceDump(); 
            }
        }
        void forceDump(){
            if (!logBuff.empty() && logFile.is_open()){
                logFile<<logBuff;
                logFile.flush();
                logBuff.clear();
            }
        }
        template <typename... Args>
        void print( Args&&... args) {
            updateBuffer("[",getTimestamp(),"]",std::forward<Args>(args)...); 
            tryDump();
        }
        template <typename... Args>
        void info( Args&&... args) {
            print("info:",std::forward<Args>(args)...);
        }
        template <typename... Args>
        void error( Args&&... args) {
            print("error:",std::forward<Args>(args)...);
        }
        template <typename... Args>
        void warn( Args&&... args) {
            print("warn:",std::forward<Args>(args)...);
        }
        // template <typename... Args>
        // static void printSync( Args&&... args) {
        //     ((std::cout << args), ...);
        //     std::cout << std::endl;// NOLINT(performance-avoid-endl)
        // }
        Logger(const Logger&)=delete;
        Logger(const Logger&&)=delete;
        Logger& operator=(Logger&)=delete;
        Logger& operator=(Logger&&)=delete;
        ~Logger(){
            try{
                forceDump();
                std::cout << "force dump";
            }catch(std::exception& e){

            }
        }
        
    };
    void setDefaultLogger(Logger& logger){
        Logger::defaultLogger=&logger;
    }
    Logger* getDefaultLogger(){
      
        return Logger::defaultLogger;
    }
    template <typename... Args>
    void infoDef( Args&&... args) {
        auto *defLogger= getDefaultLogger();
        if(defLogger)
            getDefaultLogger()->info(std::forward<Args>(args)...);
    }
    template <typename... Args>
    void errorDef( Args&&... args) {
        auto *defLogger= getDefaultLogger();
        if(defLogger)
            getDefaultLogger()->error(std::forward<Args>(args)...);
    }
    template <typename... Args>
    void warnDef( Args&&... args) {
        auto *defLogger= getDefaultLogger();
        if(defLogger)
            getDefaultLogger()->warn(std::forward<Args>(args)...);
    }
}

class NonBlockReadError:public std::exception{
     public:
    const char* what() const noexcept override {
        return "operation would block or is not supported by the filesystem";
    }
};
class ScopeGuard{
    std::move_only_function<void()> action;
    bool dismissed = false;

  public:
   ScopeGuard(std::move_only_function<void()> cleanup):action(std::move(cleanup)){}
   void disable(){
    dismissed = true;

   }
   void enable(){
    dismissed=false;
   }
   ~ScopeGuard(){
      if (!dismissed ) {
            action(); 
        }
   }
};

template<class... Ts> struct Overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> Overloaded(Ts...) -> Overloaded<Ts...>;

template<typename T ,typename ... allowed>
concept IsOneof=(std::is_same_v<T, allowed>||...);



#endif