// #ifndef COROUTINE_HPP
// #define COROUTINE_HPP
// #include <coroutine>
// #include <absl/hash/hash.h>
// #include<fdObject.hpp>
// namespace EpollInternals{
//     //i will change handlers to coroutine perfectly mimic architecture;
//     namespace Coro {
    
//         // NOLINTBEGIN(readability-identifier-naming)
//         struct StateTask{
//             struct promise_type{  
//                 std::coroutine_handle<> waiter{};
                
//                 StateTask get_return_object(){    
//                     return StateTask{std::coroutine_handle<promise_type>::from_promise(*this)};
//                 };
//                 std::suspend_always initial_suspend() { return {}; }      
//                 std::suspend_always final_suspend() noexcept { return {}; }
//                 void unhandled_exception() { std::terminate(); }
//                 void return_void(){
                    
//                 }
                
//             };
//             StateTask(const StateTask&)=delete;
//             StateTask& operator=(const StateTask& )=delete;
//             StateTask& operator=(StateTask&& other)noexcept{
//                 if(this!=&other){
//                     if(handle)handle.destroy();
//                     handle=other.handle;
//                     other.handle=nullptr;
                    
//                 }
//                 return *this;
//             };
//             ~StateTask(){if(handle) handle.destroy();}
//             std::coroutine_handle<promise_type> handle;
//             StateTask(std::coroutine_handle<promise_type> handle ):handle(handle){}
//             StateTask(StateTask&& other) noexcept : handle(other.handle) {
//                 other.handle = nullptr;  
//             }
            
            
            
            
//         };
//         struct AsyncEpollInfo{
//            EpollMan* epollMan;

//         };
//         struct AsyncEpollExec{
            
//             public:
//             bool await_ready(std::coroutine_handle<StateTask::promise_type> handle)const noexcept{
//                 return true; 
//             }
//             void await_suspend(std::coroutine_handle<> h){
                
//             };
//             void await_resume(std::coroutine_handle<> h) const noexcept {
                
//                 h.resume();   
//             }                   
            
            
//         };
//         // NOLINTEND(readability-identifier-naming)
        
        
//     }
//     }
// #endif