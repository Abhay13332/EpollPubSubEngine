#ifndef EPOLL_HPP
#define EPOLL_HPP
#include <array>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <fdObject.hpp>
#include <functional>
#include <iostream>
#include <magic_enum.hpp>
#include <stdexcept>
#include <sys/epoll.h>
#include <unistd.h>
#include <util.hpp>
#include <vector>
#include<coro.hpp>
#include <memory> 
namespace EpollInternals {

template <typename T>
concept isEpollSatisfy = std::derived_from<T, FDEPOLLRL::EpollSatisfy<T>>;
template<typename T>
concept isEpollSatisfyRDH = std::derived_from<T, FDEPOLLRL::EpollSatisfyRDH<T>>;

class EpollMan;
class EpollEvent;

template <isEpollSatisfy T> class EpollEventGen;
namespace epollFlags {
class EpollModFlags;
}

inline uint32_t operator|=(uint32_t& lhs, epollFlags::EpollModFlags rhs);
inline uint32_t operator&(uint32_t& lhs, epollFlags::EpollModFlags rhs);
inline uint32_t operator&=(uint32_t& lhs, epollFlags::EpollModFlags rhs);
namespace epollFlags {
class EpollOpenFlags {
    friend class EpollInternals::EpollMan;
    static int getValue(const EpollOpenFlags& obj) {
        return obj.val;
    }
    int val;

  public:
    EpollOpenFlags(int val) : val(val) {}
    EpollOpenFlags operator|(const EpollOpenFlags& oth) noexcept {
        return val | oth.val;
    }
};
class EpollModFlags {
    friend class EpollInternals::EpollMan;
    template <isEpollSatisfy T> friend class EpollInternals::EpollEventGen;
    friend class EpollInternals::EpollEvent;
    friend uint32_t EpollInternals::operator|=(uint32_t& lhs, epollFlags::EpollModFlags rhs);
    friend uint32_t EpollInternals::operator&(uint32_t& lhs, epollFlags::EpollModFlags rhs);
    friend uint32_t EpollInternals::operator&=(uint32_t& lhs, epollFlags::EpollModFlags rhs);

    static int getValue(const EpollModFlags& obj) {
        return obj.val;
    }

    int val;

  public:
    constexpr EpollModFlags(int val) : val(val) {}
    EpollModFlags operator|(const EpollModFlags& oth) const noexcept {
        return val | oth.val;
    }
    uint32_t operator&=(const uint32_t& oth) const noexcept {
        return static_cast<uint32_t>(val & oth);
    }
    EpollModFlags operator~() const noexcept {
        return ~val;
    }
    explicit operator int() const {
        return val;
    }
    constexpr bool operator==(const EpollModFlags& oth) const {
        return val == oth.val;
    }
};

const static EpollOpenFlags createcloseonExec = EPOLL_CLOEXEC;
using csEms = EpollModFlags;
constexpr static csEms forReadble = EPOLLIN;
constexpr static csEms forWriteable = EPOLLOUT;
constexpr static csEms foHalfClose = EPOLLRDHUP;
constexpr static csEms forPriorityData = EPOLLPRI;
constexpr static csEms forEpollErr = EPOLLERR;
constexpr static csEms forEpollHup = EPOLLHUP;
// EPOLLERR and EPOLLHUP are always reported, even if not set in events, and do
// not need to be set in events.
constexpr static csEms forEdgeTriggered = EPOLLET;
constexpr static csEms forOneShot = EPOLLONESHOT;
constexpr static csEms forExclusiveWakeup = EPOLLEXCLUSIVE;
//

}; // namespace epollFlags
inline uint32_t operator|=(uint32_t& lhs, epollFlags::EpollModFlags rhs) {
    lhs |= static_cast<uint32_t>(rhs.val);

    return lhs;
}

inline uint32_t operator&(uint32_t& lhs, epollFlags::EpollModFlags rhs) {
    return lhs & static_cast<uint32_t>(rhs.val);
};
inline uint32_t operator&=(uint32_t& lhs, epollFlags::EpollModFlags rhs) {
    return lhs &= rhs.val;
}

enum class handlerIndex : int8_t {
    Read,
    Write,
    Priority,
    SocketDisconnect,
    EpollHup,
    Err,
    TermStatusH,
    defaultCleaner,
};
constexpr handlerIndex gethandlerIndex(const epollFlags::csEms val) {
    using namespace epollFlags;
    if (val == forReadble)
        return handlerIndex::Read;
    if (val == forWriteable)
        return handlerIndex::Write;
    if (val == forPriorityData)
        return handlerIndex::Priority;
    if (val == foHalfClose)
        return handlerIndex::SocketDisconnect;
    if (val == forEpollErr)
        return handlerIndex::Err;
    if (val == forEpollHup)
        return handlerIndex::EpollHup;
    
    throw std::runtime_error("not availiblie");
}
const int gethandlerIndexInt(const epollFlags::csEms val){
    return static_cast<int>(gethandlerIndex(val));
}
template <typename T> T* getDataEpollEvent(EpollEvent* event);
class EpollRdHup {};
class NoEpollRdHup {};
class EpollEvent {

    template <isEpollSatisfy T> friend class EpollEventGen;
    friend class EpollMan;
    friend class EpollEventListenerModifier;
    template <typename T> friend T* getDataEpollEvent(EpollEvent* event);
    epoll_event event;
    int fdRef;
    void* data = nullptr; //
    bool isRDHUP = false;
    bool readyForClean = false;
    bool TermStatus = false;
    std::array<std::move_only_function<void(void*, EpollEvent*)>,
               magic_enum::enum_count<handlerIndex>()>
        handler;

    EpollEvent() : event{0}, fdRef(-1), data(0) {
        event.data.ptr = this;
    }
    EpollEvent(int fd, epollFlags::csEms events) : event{0}, fdRef(fd) {
        event.events = epollFlags::csEms::getValue(events);
        event.data.ptr = this;
    }
    EpollEvent(epollFlags::csEms events) : event{0}, fdRef(-1) {
        event.events = epollFlags::csEms::getValue(events);
        event.data.ptr = this;
    }
    EpollEvent(int fd) : event{0}, fdRef(fd) {
        event.data.ptr = this;
    }
    void trycleanUp() {
        if (readyForClean && handler.back()) {
            handler.back()(data, this);
        }
    }
    void runTermStatusH(){
        int idx=static_cast<size_t> (handlerIndex::TermStatusH);
        if(handler[idx]){
            handler[idx](data,this);
        }
    }

    void setonTermStatus(){
        TermStatus= true;
    }
    void runEvent(uint32_t activeFlags) {
        using namespace epollFlags;

        if (activeFlags & forReadble) {
            int idx = gethandlerIndexInt(forReadble);
            if (handler[idx]) {

                handler[idx](data, this);
            }
        }
        if (activeFlags & forWriteable) {
            int idx = gethandlerIndexInt(forWriteable);
            if (handler[idx]) {
                handler[idx](data,this);
            }
        }
        if (activeFlags & forPriorityData) {
            int idx = gethandlerIndexInt(forPriorityData);
            if (handler[idx]) {
                handler[idx](data, this);
            }
        }
        if ((activeFlags & foHalfClose) || isRDHUP) {
            isRDHUP = true;
            int idx = gethandlerIndexInt(foHalfClose);
            if (handler[idx]) {
                handler[idx](data, this);
            }
        }
        if (activeFlags & (forEpollErr | forEpollHup)) {
            if (activeFlags & forEpollErr) {

                int idx = gethandlerIndexInt(forEpollErr);
                if (handler[idx]) {
                    handler[idx](data, this);
                    return;
                }
            }
            if (activeFlags & forEpollHup) {

                int idx = gethandlerIndexInt(forEpollHup);
                if (handler[idx]) {
                    handler[idx](data, this);
                    return;
                } else {
                    int idx = gethandlerIndexInt(forEpollErr);
                    if (handler[idx]) {
                        handler[idx](data, this);
                        return;
                    }
                }
            }
            // default cleanup;
        }
        // handler for epollTerm must above on try cleanup
        runTermStatusH();
        trycleanUp();
    }

  public:
    int getFd() {
        return fdRef;
    }
    std::expected<EpollRdHup, NoEpollRdHup> getRdhupStatus() {
        if (isRDHUP) {
            return EpollRdHup{};
        } else {
            return std::unexpected(NoEpollRdHup{});
        }
    }
    void setCleanup() {
        readyForClean = true;
    }
    // EpollEvent(EpollEvent&&)=default;
    
};
template <typename T> T* getDataEpollEvent(EpollEvent* event) {
    return static_cast<T*>(event->data);
}

class EpollEventListenerModifier {
    EpollEvent* event;
    EpollMan* epollMan;

  public:
    EpollEventListenerModifier(EpollEvent* event,EpollMan* epollMan) : event(event) ,epollMan(epollMan){
        if (this->event == nullptr || this->epollMan == nullptr) {
            throw std::invalid_argument(
                "Pointer cannot be null in EpollEventListenerModifier constructor");
        }
    }
    EpollEventListenerModifier* disableEpollEdgeTriggered() {
        event->event.events &= ~epollFlags::forEdgeTriggered;
        return this;
    }
    EpollEventListenerModifier* enableEpollEdgeTriggered() {
        event->event.events |= epollFlags::forEdgeTriggered;
        return this;
    }
    EpollEventListenerModifier* enabletriggerOnlyOnce() {
        event->event.events |= epollFlags::forOneShot;
        return this;
    }
    EpollEventListenerModifier* disabletriggerOnlyOnce() {
        event->event.events &= ~epollFlags::forOneShot;
        return this;
    }
    EpollEventListenerModifier* enableReadEvent() {
        event->event.events |= epollFlags::forReadble;
        return this;
    }
    EpollEventListenerModifier* enableWriteEvent() {
        event->event.events |= epollFlags::forWriteable;
        return this;
    }
    EpollEventListenerModifier* disableReading() {
        event->event.events &= ~epollFlags::forReadble;
        return this;
    }
    EpollEventListenerModifier* disableWriteEvent() {
        event->event.events &= ~epollFlags::forWriteable;
        return this;
    }
    EpollEventListenerModifier* enablePriorityEvent() {
        event->event.events |= epollFlags::forPriorityData;
        return this;
    }
    EpollEventListenerModifier* disablePriorityEvent() {
        event->event.events &= ~epollFlags::forPriorityData;
        return this;
    }
    EpollEventListenerModifier* enableSocketDisconnectEvent() {
        event->event.events |= epollFlags::foHalfClose;
        return this;
    }
    EpollEventListenerModifier* disableSocketDisconnectEvent() {
        event->event.events &= ~epollFlags::foHalfClose;
        return this;
    }
    void modifyinEpoll();
};
template <isEpollSatisfy T> class EpollEventGen {
    std::unique_ptr<EpollEvent> EpollManevent;
    EpollMan* epoll;

    friend class EpollMan;
    EpollEventGen(int fd, epollFlags::csEms events, EpollMan* epoll)
        : EpollManevent(new EpollEvent(fd, events)), epoll(epoll) {}
    EpollEventGen(epollFlags::csEms events, EpollMan* epoll)
        : EpollManevent(new EpollEvent(events)), epoll(epoll) {}
    EpollEventGen(int fd, EpollMan* epoll) : EpollManevent(new EpollEvent(fd)), epoll(epoll) {}
    EpollEventGen(EpollMan* epoll) : EpollManevent(new EpollEvent()), epoll(epoll) {}
    EpollEventGen(EpollEventGen&)=delete;
    EpollEventGen& operator=(EpollEventGen&)=delete;
    EpollEventGen(EpollEventGen&&)=default;
    EpollEventGen& operator=(EpollEventGen&&)=default;
    
  public:
    EpollEvent* addEvent();
    EpollEvent* modifyEvent();
    EpollEvent* deleteEvent();
    
    EpollEventGen* addObject(void* data) {
        EpollManevent->data = data;
        return this;
    }
    EpollEventGen* triggerModeEdge() {
        EpollManevent->event.events |= epollFlags::forEdgeTriggered;
        return this;
    }
    EpollEventGen* triggerOnlyOnce() {
        EpollManevent->event.events |= epollFlags::forOneShot;
        return this;
    }
    EpollEventGen* triggerExclusiveWakeup() {
        EpollManevent->event.events |= epollFlags::forExclusiveWakeup;
        return this;
    }
    EpollEventGen* onReading(std::move_only_function<void(T*, EpollEvent*)> handler,
                             bool turnon = true) {

        if (turnon)
            EpollManevent->event.events |= epollFlags::forReadble;

        assignHandler(gethandlerIndex(epollFlags::forReadble), std::move(handler));
        return this;
    }
    EpollEventGen* onWrite(std::move_only_function<void(T*, EpollEvent*)> handler,
                           bool turnon = true) {
        if (turnon)
            EpollManevent->event.events |= epollFlags::forWriteable;
        assignHandler(gethandlerIndex(epollFlags::forWriteable), std::move(handler));
        return this;
    }
    //cleanup should occur only here in other handler they can set setcleaup to trigger this one
    EpollEventGen* onCleanup(std::move_only_function<void(T*, EpollEvent*)> handler
                             ) {

        assignHandler(handlerIndex::defaultCleaner, std::move(handler));
        return this;
    }
    EpollEventGen* onTermStatus(std::move_only_function<void(T*, EpollEvent*)> handler)
    requires isEpollSatisfyRDH<T>{
        EpollManevent->handler[static_cast<int>(handlerIndex::TermStatusH)] =
            [handler = std::move(handler)](void* data, EpollEvent* eventObj) mutable {
                if (handler && data && static_cast<T*>(data)->terminationStatus()) {
                    handler(static_cast<T*>(data), eventObj);
                }
            };
        return this;
    }
    EpollEventGen* onPriorityData(std::move_only_function<void(T*, EpollEvent*)> handler,
                                  bool turnon = true) {
        if (turnon)
            EpollManevent->event.events |= epollFlags::forPriorityData;
        assignHandler(gethandlerIndex(epollFlags::forPriorityData), std::move(handler));
        return this;
    }
    EpollEventGen* onHalfClose ( std::move_only_function<void(T*, EpollEvent*)> handler,
            bool turnon = true) requires isEpollSatisfyRDH<T>{
        if (turnon)
            EpollManevent->event.events |= epollFlags::foHalfClose;
        EpollManevent->handler[static_cast<int>(gethandlerIndex(epollFlags::foHalfClose))] =
            [handler = std::move(handler)](void* data, EpollEvent* eventObj) mutable {
                if (handler && data && static_cast<T*>(data)->isWriteComplete()) {
                    handler(static_cast<T*>(data), eventObj);
                }
            };
        return this;
    }
    EpollEventGen* onErr(std::move_only_function<void(T*, EpollEvent*)> handler) {
        assignHandler(gethandlerIndex(epollFlags::forEpollErr), std::move(handler));
        return this;
    }
    EpollEventGen* onDisc(std::move_only_function<void(T*, EpollEvent*)> handler) {
        assignHandler(gethandlerIndex(epollFlags::forEpollErr), std::move(handler));
        return this;
    }
  
  private:
    void assignHandler(handlerIndex target,
                       std::move_only_function<void(T*, EpollEvent*)> handler) {
        EpollManevent->handler[static_cast<int>(target)] = (typeEraser(std::move(handler)));
    }
    std::move_only_function<void(void*, EpollEvent*)>
    typeEraser(std::move_only_function<void(T*, EpollEvent*)>&& handler) {
        return [innerhandler = (std::move(handler))](void* data, EpollEvent* eventObj) mutable {
            if (innerhandler && data) {
                innerhandler(static_cast<T*>(data), eventObj);
            }
        };
    }

    // static Edge
};


void printEpollFlags(uint32_t events) {
    DEBUG_RUN(std::cout << "Bitmask " << events << " contains:\n";

              if (events & EPOLLIN) std::cout << "  - EPOLLIN (1)\n";
              if (events & EPOLLPRI) std::cout << "  - EPOLLPRI (2)\n";
              if (events & EPOLLOUT) std::cout << "  - EPOLLOUT (4)\n";
              if (events & EPOLLERR) std::cout << "  - EPOLLERR (8)\n";
              if (events & EPOLLHUP) std::cout << "  - EPOLLHUP (16)\n";
              if (events & EPOLLRDHUP) std::cout << "  - EPOLLRDHUP (8192)\n";
              if (events & EPOLLET) std::cout << "  - EPOLLET\n";
              if (events & EPOLLONESHOT) std::cout << "  - EPOLLONESHOT\n";);
}


template<typename Func,typename objTp,typename ...CpArgs>
concept EpollRegFuncaptIf=std::invocable<Func,objTp*,EpollEvent*,CpArgs...>;
template<typename Func,typename T>
concept EpollEvgenBuilder=std::invocable<Func, EpollEventGen<T>&>;
class EUtil{
    public:
        template<isEpollSatisfy T,typename Func,typename... CpArgs>
        requires EpollRegFuncaptIf<Func,T, CpArgs...>
        static  std::move_only_function<void(T*, EpollEvent*)> deleg(Func&& function,CpArgs&& ...cpArgs){
            return [fn=std::forward<Func>(function),cpTuple=std::make_tuple(std::forward<CpArgs>(cpArgs)...)]
            (T* data, EpollEvent* epollEv)mutable{
               std::apply([&fn,data,epollEv](auto&&... cpArgs)mutable{
                    std::invoke(fn,data,epollEv,std::forward<decltype(cpArgs)>(cpArgs)...);
               }, cpTuple);
            };
        };
};
class EpollMan {
    FDEPOLLRL::FileDesc epollFd;
    std::vector<epoll_event> events;

  public:
    EpollMan(const size_t maxEvents = 10, const epollFlags::EpollOpenFlags val = 0) {

        int flags = epollFlags::EpollOpenFlags::getValue(val);
        int res = epoll_create1(flags);
        if (res == -1) {
            switch (errno) {
            case EINVAL:
                throw std::runtime_error("Invalid value in flags");
            case EMFILE:
                throw std::runtime_error("The per-process limit on the number of epoll "
                                         "instances has been reached");
            case ENFILE:
                throw std::runtime_error("The system-wide limit on the total number of "
                                         "open files has been reached");
            default:
                throw std::runtime_error("Unknown error occurred while creating epoll instance: " +
                                         std::to_string(errno));
            }
        }
        epollFd = res;
        events = std::vector<epoll_event>(maxEvents);
    }
    EpollMan(EpollMan&) = delete;
    EpollMan& operator=(EpollMan&) = delete;
    EpollMan(EpollMan&&) = default;
    EpollMan& operator=(EpollMan&&) = default;

    void runEventLoop() {
        int i = 0;
        // int totale=0;
        while (true) {

            int eventCount = epoll_wait(epollFd.get(), events.data(), events.size(), -1);
            // totale+=eventCount;

            if (eventCount == -1) {
                switch (errno) {
                case EBADF:
                    throw std::runtime_error("Invalid epoll file descriptor");
                case EINTR:
                    throw std::runtime_error("The call was interrupted by a signal handler "
                                             "before any events were available");
                case EINVAL:
                    throw std::runtime_error("The epoll file descriptor is not valid or "
                                             "maxevents is less than or equal to zero");
                default:
                    throw std::runtime_error("Unknown error occurred while waiting for events: " +
                                             std::to_string(errno));
                }
            }
            for (int i = 0; i < eventCount; i++) {
                EpollEvent* eventData = static_cast<EpollEvent*>(events[i].data.ptr);
                (printEpollFlags(events[i].events));
                eventData->runEvent(events[i].events);
            }
            debug::print("runloop");
        }
    }
    void addEvent(EpollEvent* epollEvent) {
        int res = epoll_ctl(epollFd.get(), EPOLL_CTL_ADD, epollEvent->fdRef, &epollEvent->event);
        if (res == -1) {
            switch (errno) {
            case EBADF:
                throw std::runtime_error("Invalid epoll file descriptor");
            case EEXIST:
                throw std::runtime_error("The specified file descriptor is already "
                                         "registered with this epoll instance");
            case EINVAL:
                throw std::runtime_error("Invalid epoll file descriptor or event structure");
            case ENOENT:
                throw std::runtime_error("The specified file descriptor is not "
                                         "registered with this epoll instance");
            case ENOMEM:
                throw std::runtime_error("Insufficient kernel memory to add the event");
            default:
                throw std::runtime_error("Unknown error occurred while adding event: " +
                                         std::to_string(errno));
            }
        }
    }
    void modifyEvent(EpollEvent* epollEvent) {
        int res = epoll_ctl(epollFd.get(), EPOLL_CTL_MOD, epollEvent->fdRef, &epollEvent->event);
        if (res == -1) {
            switch (errno) {
            case EBADF:
                throw std::runtime_error("Invalid epoll file descriptor");
            case EEXIST:
                throw std::runtime_error("The specified file descriptor is already "
                                         "registered with this epoll instance");
            case EINVAL:
                throw std::runtime_error("Invalid epoll file descriptor or event structure");
            case ENOENT:
                throw std::runtime_error("The specified file descriptor is not "
                                         "registered with this epoll instance");
            case ENOMEM:
                throw std::runtime_error("Insufficient kernel memory to modify the event");
            default:
                throw std::runtime_error("Unknown error occurred while modifying event: " +
                                         std::to_string(errno));
            }
        }
    }
    void deleteEvent(int fd) {
        int res = epoll_ctl(epollFd.get(), EPOLL_CTL_DEL, fd, nullptr);
        if (res == -1) {
            switch (errno) {
            case EBADF:
                throw std::runtime_error("Invalid epoll file descriptor");
            case EINVAL:
                throw std::runtime_error("Invalid epoll file descriptor");
            case ENOENT:
                throw std::runtime_error("The specified file descriptor is not "
                                         "registered with this epoll instance");
            default:
                throw std::runtime_error("Unknown error occurred while deleting event: " +
                                         std::to_string(errno));
            }
        }
    }
    void deleteEvent(EpollEvent* epollEvent) {
        this->deleteEvent(epollEvent->fdRef);
    }
    template <isEpollSatisfy T> EpollEventGen<T> createEventObj(int fd, epollFlags::csEms events) {

        return EpollEventGen<T>(fd, events, this);
    }
    template <isEpollSatisfy T> EpollEventGen<T> createEventObj(int fd) {
        return EpollEventGen<T>(fd, this);
    }
    template <isEpollSatisfy T> EpollEventGen<T> createEventObj(epollFlags::csEms events) {

        return EpollEventGen<T>(events, this);
    }
    template <isEpollSatisfy T> EpollEventGen<T> createEventObj(T* fdObj) {
        std::pair<int, epollFlags::EpollModFlags> info = fdObj->getEpollInfo();
        return std::move(*(EpollEventGen<T>(info.first, info.second, this).addObject(fdObj)));
    }

    template <isEpollSatisfy T,typename Func>
    requires EpollEvgenBuilder<Func,T>
     EpollEvent* createEventObjLinIF
    (T* fdObj,Func builder) {
        std::pair<int, epollFlags::EpollModFlags> info = fdObj->getEpollInfo();
        EpollEventGen<T> epollGen(info.first, info.second, this);
        epollGen.addObject(fdObj);
        builder(epollGen);
        return epollGen.addEvent();
    }
};
template <isEpollSatisfy T> EpollEvent* EpollEventGen<T>::addEvent() {

    this->epoll->addEvent(this->EpollManevent.get());
    return this->EpollManevent.release();
}
template <isEpollSatisfy T> EpollEvent* EpollEventGen<T>::modifyEvent() {
    this->epoll->modifyEvent(this->EpollManevent.get());
    
    return this->EpollManevent.release();
}
template <isEpollSatisfy T> EpollEvent* EpollEventGen<T>::deleteEvent() {
    this->epoll->deleteEvent(this->EpollManevent.get());
    
    return this->EpollManevent.release();
}
void EpollEventListenerModifier::modifyinEpoll() {
    this->epollMan->modifyEvent(this->event);
}
namespace EpDef{

    template <typename T>
    using EEG=EpollEventGen<T>;
}
} // namespace EpollInternals

#endif