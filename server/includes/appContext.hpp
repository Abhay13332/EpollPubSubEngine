#ifndef APP_CONTEXT_HPP
#define APP_CONTEXT_HPP
#include <epoll.hpp>

class AppContext;
template<EpollInternals::onlyDataObj T>
class AppListener{
    using EpollEvent=EpollInternals::EpollEvent;
    friend class AppContext;
    T* fdObj;
    AppContext& appCtx;
    public:
    AppListener(AppContext&ctx,T* obj):appCtx(ctx),fdObj(obj){}
    virtual ~AppListener()=default;
    virtual void onReading(T*,EpollEvent*,AppContext* ctx)=0;
    virtual void onWrite(T*,EpollEvent*,AppContext* ctx){};
    virtual void onCleanup(T*,EpollEvent*,AppContext* ctx){};
    virtual void onRdhup(T*,EpollEvent*,AppContext* ctx){};
    virtual void onEpollErr(T*,EpollEvent*,AppContext* ctx){};

};
class AppContext{
    using EpollEvent=EpollInternals::EpollEvent;
    using EpollMan=EpollInternals::EpollMan;
    
    EpollMan* epollManRef;
    public:
    AppContext(EpollMan* epollMan):epollManRef(epollMan){
        
    }
    template<EpollInternals::onlyDataObj T>
    // create(AppListener<T>  )

};

#endif