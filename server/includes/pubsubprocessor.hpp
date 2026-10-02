#ifndef PUB_SUB_PROCESSOR_HPP
#define PUB_SUB_PROCESSOR_HPP
#include <pubsub.hpp>
#include <parallel_hashmap/phmap.h>
#include<gtest/gtest.h>
#include<base64/base64.hpp>
namespace PubSubEngine {
namespace protoState{
    class State{
        protected:
        virtual std::string info()noexcept=0;
        virtual ~State()=default;  
    };
    class DataNotArrived:public State{
        public:
        int expected = -1;
        int arrived = -1;
        public:
        DataNotArrived() = default;
        DataNotArrived(int exp, int arr) : expected(exp), arrived(arr) {}
        std::string info()  noexcept override {
            if (expected == -1) {
                return "operation can't perform data not arrived yet";
            }
            return std::format("operation can't perform data not arrived yet expected={},arrived={}",
                expected, arrived);
            }
      };
    class DataArrived:public State {
        public:
          int stPos;
          int size;
          int endPos;
          DataArrived(int stpos,int size):stPos(stpos),size(size),endPos(stpos+size-1){};
          std::string info()noexcept{
            return "size bytes  arrived";
          }
    };  
    class SizeBytesNotArrived:public State {
        public:
          SizeBytesNotArrived()=default;
          std::string info()noexcept{
            return "size bytes not arrived";
          }
    };
    class InvalidProtoMSG :public State{
        public:
        std::string invalidReq="";
        public:
        InvalidProtoMSG(const std::string &invalidReq="-"):invalidReq("- "+invalidReq){} ;
        std::string info()  noexcept  {
            return std::format("invalid request from user {}",invalidReq);
        }
      };
    class NotInitialized:public State {
        public:
        std::string invalidReq="";
        public:
        NotInitialized(const std::string &invalidReq=""):invalidReq(invalidReq){} ;
        std::string info()  noexcept  {
            return std::format("command before Initilization command {}",invalidReq);
        }
      };
        class InvalidSubCmd:public State {
        public:
        std::string invalidReq="";
        public:
        InvalidSubCmd(const std::string &invalidReq=""):invalidReq(invalidReq){} ;
        std::string info()  noexcept  {
            return std::format("Invalid Subscriber Command {}",invalidReq);
        }
      };
        class InvalidPubCmd:public State {
        public:
        std::string invalidReq="";
        public:
        InvalidPubCmd(const std::string &invalidReq=""):invalidReq(invalidReq){} ;
        std::string info()  noexcept  {
            return std::format("Invalid Publisher Command {}",invalidReq);
        }
      };
    class ReadBuffEmpty:public State{
        public:
        ReadBuffEmpty()=default;
        std::string info()noexcept{
            return std::format("empty read buffer");
        }
    } ;
    class CommandProcessed:public State{
    public:
        std::string someInfo;
        CommandProcessed(const std::string &someInfo=""):someInfo(someInfo){};
        std::string info()noexcept {
            return std::format("Command Processed");
        }
    };
    using DataStatus=std::variant<DataNotArrived,ReadBuffEmpty,SizeBytesNotArrived,DataArrived>;
    using AssignData=std::variant<std::unique_ptr<PubsubIF>,DataNotArrived,InvalidProtoMSG,ReadBuffEmpty,SizeBytesNotArrived,NotInitialized>;
    using CommandStatus=std::variant<CommandProcessed,DataNotArrived,InvalidProtoMSG,ReadBuffEmpty,SizeBytesNotArrived,InvalidSubCmd,InvalidPubCmd>;
    class NegativeValue:public State {
        public:
          NegativeValue()=default;
          std::string info()noexcept{
            return "Negative value is Provdied";
          }
    };

}
static std::move_only_function<int(int)> posGen(int initPos=0){
        return [pos=initPos](int forward)mutable{
            int curr=pos;
            pos+=forward;
            return curr;
        };
    }

enum class Tokentype:std::uint8_t{
    PUB,
    SUB,
    CREATE,
    ADDTPC,
    REMTPC,
    TOPIC,
    MSGONTPC,
    MSGIS,
    MSG,
    LISTTPC,
};
class Token{
    public:
    Tokentype ttp;
    std::string data;
    Token(Tokentype ttp,std::string data=""):ttp(ttp),data(std::move(data)){}
};
class InvalidTokens{
    public:
    std::string invalidPart="";
    public:
    InvalidTokens(const std::string &invalidPart=""):invalidPart(invalidPart){} ;
    std::string info()  noexcept  {
        return std::format("invalid Token {}",invalidPart);
    }
};
class TokenList{
    public:
    std::vector<Token> Tkl;
    void addTkn(Tokentype ttp,std::string data=""){
        Tkl.emplace_back(ttp,std::move(data));
    }
    static std::expected<TokenList,InvalidTokens> getTkList(std::string_view st){
        auto pos=posGen();
        TokenList tkList;
        while(pos(0)!=st.size()){
            if(st[pos(0)]==' '){
                pos(1);
            }else if((pos(0)+3<st.size()) && st.substr(pos(0),4)=="PUB "){
                tkList.addTkn(Tokentype::PUB);
                pos(4);
            }else if((pos(0)+3<st.size()) && st.substr(pos(0),4)=="SUB "){
                tkList.addTkn(Tokentype::SUB);
                pos(4);
            }else if((pos(0)+5<st.size()) && st.substr(pos(0),6)=="CREATE"){
                tkList.addTkn(Tokentype::CREATE);
                pos(6);
            }else if((pos(0)+6<st.size()) && st.substr(pos(0),7)=="ADDTPC "){
                tkList.addTkn(Tokentype::ADDTPC);
                pos(7);
            }else if((pos(0)+6<st.size()) && st.substr(pos(0),7)=="REMTPC "){
                tkList.addTkn(Tokentype::REMTPC);
                pos(7);
            }else if((pos(0)+6<st.size()) && st.substr(pos(0),7)=="LISTTPC "){
                tkList.addTkn(Tokentype::LISTTPC);
                pos(7);
            }else if((pos(0)+8<st.size()) && st.substr(pos(0),9)=="MSGONTPC "){
                tkList.addTkn(Tokentype::MSGONTPC);
                pos(9);
            }else if((pos(0)+6<st.size()) && st.substr(pos(0),6)=="MSGIS "){
                tkList.addTkn(Tokentype::MSGIS);
                pos(6);
            }else if(tkList.Tkl.size()>0 && tkList.Tkl.back().ttp==Tokentype::MSGIS) {
                        std::string decodedData=base64::from_base64(st.substr(pos(0)));
                        tkList.addTkn(Tokentype::MSG,decodedData);
            }else if(tkList.Tkl.size()>0 ){
            auto lasttkn=(tkList.Tkl.back().ttp);
            if(lasttkn!=Tokentype::ADDTPC||
                lasttkn!=Tokentype::REMTPC||lasttkn!=Tokentype::MSGONTPC
            )return std::unexpected(InvalidTokens());
            std::string tkData;
            while(pos(0)!=st.size() && st[pos(0)]!=' '){
                tkData+=st[pos(1)];
            }
            tkList.addTkn(Tokentype::TOPIC,tkData);
        }else{
            return std::unexpected(InvalidTokens());
        }
    }
    return tkList;
    
}


};
enum class CommandTp:std::uint8_t{
    PUBCREATE,//PUB CREATE
    SUBCREATE,//SUB CREATE
    ADDTPC,//ADDTPC 
    REMTPC,
    LISTTPC,
    PUBMSG,//MSGONTPC TOPIC  MSGIS MSG
};
class Command{
    public:
        CommandTp tp;
        Command(CommandTp tp):tp(tp){};
        virtual ~Command()=default;
};
class TopicModCmd:public Command{
    public:    
        std::string topic;
        TopicModCmd(CommandTp tp,const std::string& topic):topic(topic),Command(tp){}

};
class TopicMsgCmd:public Command{
    public:
        std::string topic;
        std::string msg;
        TopicMsgCmd(CommandTp tp,const std::string& topic,const std::string& msg ):topic(topic),msg(msg),Command(tp){}
};
class CommandProcessor{
    public:
        static  std::expected<std::unique_ptr<Command>,protoState::InvalidProtoMSG> getCmd(TokenList& tklist){
            int len=tklist.Tkl.size();
            auto&tklref=tklist.Tkl;
            if(len==1 ){
                if(tklref[0].ttp==Tokentype::LISTTPC){
                    return std::make_unique<Command>(Command{CommandTp::LISTTPC});
                }
                return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid comand for  length 1:{}",
                        magic_enum::enum_name(tklist.Tkl[0].ttp))));
            }else if(len==2){
                auto tk1=tklist.Tkl[0].ttp;
                auto tk2=tklist.Tkl[1].ttp;
                if((tk1==Tokentype::PUB||tk2==Tokentype::SUB) && tk2==Tokentype::CREATE){
                    return std::make_unique<Command>(
                        Command{tk1==Tokentype::PUB?
                            CommandTp::PUBCREATE:CommandTp::SUBCREATE});
                }else if(tk1==Tokentype::ADDTPC && tk2==Tokentype::TOPIC){
                    return  std::make_unique<Command>(TopicModCmd(CommandTp::ADDTPC,tklist.Tkl[1].data));
                }else if(tk1==Tokentype::REMTPC && tk2==Tokentype::TOPIC){
                    return  std::make_unique<Command>(TopicModCmd(CommandTp::REMTPC,tklist.Tkl[1].data));
                }
                return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid comand for  length 2:{}",
                    magic_enum::enum_name(tklist.Tkl[0].ttp))));
                

            }else if(len==4){
                auto tk1=tklist.Tkl[0].ttp;
                auto tk2=tklist.Tkl[1].ttp;
                auto tk3=tklist.Tkl[2].ttp;
                auto tk4=tklist.Tkl[3].ttp;
                if(tk1==Tokentype::MSGONTPC && tk2==Tokentype::TOPIC && tk3==Tokentype::MSGIS && tk4==Tokentype::MSG){
                    return std::make_unique<Command>(TopicMsgCmd(CommandTp::PUBMSG,tklist.Tkl[1].data,tklist.Tkl[3].data));
                }
                return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid comand for  length 4:{}",
                    magic_enum::enum_name(tklist.Tkl[0].ttp))));
                
            }
            return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid len of Tokens:{}",len)));
        } 
};

class PSProtocol{
    static int getSize(std::string_view val){
        uint32_t bigEndval;
        std::memcpy(&bigEndval,val.data(),4);
        return static_cast<int>(ntohl(bigEndval));
    }
   
    
    static protoState::DataStatus verifyData(socketIO::SocketClient* cl){
        auto pos=posGen();
        if(!cl->ReadDataBuff.size()) return protoState::ReadBuffEmpty();
        if(cl->ReadDataBuff.size()-pos(0)+1<4) return protoState::SizeBytesNotArrived();
        std::string_view buffrf(cl->ReadDataBuff);
        int datasize=getSize(buffrf.substr(pos(4),4));
        if(cl->ReadDataBuff.size()-pos(0)+1<datasize) return  protoState::DataNotArrived();
        return protoState::DataArrived(pos(0),datasize);
        
    }
    static std::expected<TokenList, InvalidTokens> getTknMove(socketIO::SocketClient* cl,protoState::DataArrived& dataInfo){
        ScopeGuard scg([cl,dataInfo]()mutable{
            cl->ReadDataBuff.erase(0,dataInfo.stPos+ dataInfo.size-1);
        });
        std::string_view commandData=std::string_view(cl->ReadDataBuff).substr(dataInfo.stPos,dataInfo.size);
        return TokenList::getTkList(commandData);
    }
    
    public:
     static std::string  getByteFromInt(int val){
        std::string charVal(4,0); 
        uint32_t bendlen=htonl(static_cast<uint32_t>(val));
        std::memcpy(charVal.data(),&bendlen,4);
        return charVal;
    }
    static  protoState::AssignData assignType(Skcl* skcl,PubSubMan* pubSubMgr){
        socketIO::SocketClient* cl=skcl->skc.get();
        auto dataStatus=verifyData(cl);
        return std::visit(Overloaded{
            [cl,skcl,pubSubMgr](protoState::DataArrived& dataInfo)->protoState::AssignData{
                auto tkListexp=getTknMove(cl,dataInfo);
                if(!tkListexp.has_value()) return protoState::InvalidProtoMSG();
                auto cmdExp=CommandProcessor::getCmd(tkListexp.value());
                if(!cmdExp.has_value())return protoState::InvalidProtoMSG();
                auto cmd=std::move(cmdExp.value());
                if(cmd->tp!=CommandTp::SUBCREATE||cmd->tp!=CommandTp::PUBCREATE) return protoState::NotInitialized();
                if(cmd->tp==CommandTp::SUBCREATE) return std::make_unique<PubsubIF>(Subscriber(skcl,pubSubMgr));
                return std::make_unique<PubsubIF>(Publisher(skcl,pubSubMgr));
                
            },
            [](auto &otherSt)->protoState::AssignData{
                
                return otherSt;
            }
        },dataStatus);
    }
    static  protoState::CommandStatus processCmdSub(socketIO::SocketClient* cl,Subscriber* sub){
            auto dataStatus=verifyData(cl);
            return std::visit(Overloaded{
                [cl,sub](protoState::DataArrived& dataInfo)->protoState::CommandStatus{
                    auto tkListexp=getTknMove(cl,dataInfo);
                    if(!tkListexp)return protoState::InvalidProtoMSG();
                    auto cmdExp=CommandProcessor::getCmd((tkListexp.value()));
                    if(!cmdExp.has_value())return protoState::InvalidProtoMSG();
                    auto cmd=std::move(cmdExp.value());
                    if(cmd->tp==CommandTp::ADDTPC){
                        auto cmdModTpc=dynamic_cast<TopicModCmd*>(cmd.get());
                        sub->addTpc(cmdModTpc->topic);
                    }else if(cmd->tp==CommandTp::REMTPC){
                        auto cmdModTpc=dynamic_cast<TopicModCmd*>(cmd.get());
                        sub->remTpc(cmdModTpc->topic);
                    }else if(cmd->tp==CommandTp::LISTTPC){
                        sub->topicListShow();
                    }else {
                        return protoState::InvalidSubCmd();
                    }
                    return protoState::CommandProcessed();

                },
                [](auto& oth)->protoState::CommandStatus{
                    return oth;
                }
            },dataStatus);
    }
    static  protoState::CommandStatus processCmdPub(socketIO::SocketClient* cl,Publisher* pub){
            auto dataStatus=verifyData(cl);
            return std::visit(Overloaded{
                [cl,pub](protoState::DataArrived& dataInfo)->protoState::CommandStatus{
                    auto tkListexp=getTknMove(cl,dataInfo);
                    if(!tkListexp)return protoState::InvalidProtoMSG();
                    auto cmdExp=CommandProcessor::getCmd((tkListexp.value()));
                    if(!cmdExp.has_value())return protoState::InvalidProtoMSG();
                    auto cmd=std::move(cmdExp.value());
                    if(cmd->tp==CommandTp::ADDTPC){
                        auto cmdModTpc=dynamic_cast<TopicModCmd*>(cmd.get());
                        pub->addTpc(cmdModTpc->topic);
                    }else if(cmd->tp==CommandTp::REMTPC){
                        auto cmdModTpc=dynamic_cast<TopicModCmd*>(cmd.get());
                        pub->remTpc(cmdModTpc->topic);
                    }else if(cmd->tp==CommandTp::PUBMSG){
                        auto cmdModTpc=dynamic_cast<TopicMsgCmd*>(cmd.get());
                        pub->distributeMsg(cmdModTpc->topic,cmdModTpc->msg);
                    }else {
                        return protoState::InvalidSubCmd();
                    }
                    return protoState::CommandProcessed();

                },
                [](auto& oth)->protoState::CommandStatus{
                    return oth;
                }
            },dataStatus);
    }
    FRIEND_TEST(pubsubProcesser, verifySizeConversion);
    FRIEND_TEST(pubSubProcesser, verifyData);

};

void PubSubMan::publish(const std::string& topic,const std::string& msg){
    std::string pubCmd="MSGONTPC "+topic+"MSGIS "+base64::to_base64(msg);
    subs[topic].write(PSProtocol::getByteFromInt(pubCmd.length())+pubCmd);
}
}

#endif