#ifndef CMD_PROCESSOR_HPP
#define CMD_PROCESSOR_HPP
#include<functional>
#include <cstdint>
#include <netinet/in.h>
#include<string>
#include<expected>
#include <format>
#include<parallel_hashmap/phmap.h>
#include<magic_enum.hpp>
#include<base64/base64.hpp>
#include<util.hpp>
namespace PubSubSdk{
namespace protoState{
    class State{
        protected:
        virtual std::string info()noexcept=0;
        virtual ~State()=default;  
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
}
static std::move_only_function<int(int)> posGen(int initPos=0){
        return [pos=initPos](int forward)mutable{
            int curr=pos;
            pos+=forward;
            return curr;
        };
}

enum class Tokentype:std::uint8_t{
    STATUS,
    STVAL,
    MSGONTPC,
    TOPIC,
    MSGIS,
    MSG,
    LIST,
    
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
    static bool isConvertibleInt(std::string_view str) {
        int value;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);    
        return ec == std::errc{} && ptr == str.data() + str.size();
    }

    static std::expected<TokenList,InvalidTokens> getTkList(std::string_view st){
        auto pos=posGen();
        TokenList tkList;
        while(pos(0)!=st.size()){
            if(st[pos(0)]==' '){
                pos(1);
            }else if((pos(0)+4<st.size()) && st.substr(pos(0),5)=="LIST "){
                 pos(5);
                 tkList.addTkn(Tokentype::LIST,std::string(st.substr(pos(0))));
                 pos(st.size()-pos(0));
            }
            else if((pos(0)+6<st.size()) && st.substr(pos(0),7)=="STATUS "){
                tkList.addTkn(Tokentype::STATUS);
                pos(7);
            }else if(tkList.Tkl.size()==1 && tkList.Tkl[0].ttp==Tokentype::STATUS && isConvertibleInt(st.substr(pos(0)))){
                tkList.addTkn(Tokentype::STVAL,std::string(st.substr(pos(0))));
                pos(st.size()-pos(0));
            }else if((pos(0)+8<st.size()) && st.substr(pos(0),9)=="MSGONTPC "){
                tkList.addTkn(Tokentype::MSGONTPC);
                pos(9);
            }else if((pos(0)+5<st.size()) && st.substr(pos(0),6)=="MSGIS "){
                tkList.addTkn(Tokentype::MSGIS);
                pos(6);
            }else if(tkList.Tkl.size()>0 && tkList.Tkl.back().ttp==Tokentype::MSGIS) {
                        std::string decodedData=base64::from_base64(st.substr(pos(0)));
                        tkList.addTkn(Tokentype::MSG,decodedData);
                        pos(st.size()-pos(0));
            }else if(tkList.Tkl.size()>0 ){
            auto lasttkn=(tkList.Tkl.back().ttp);
            if(lasttkn!=Tokentype::MSGONTPC)return std::unexpected(InvalidTokens());
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
    STATUSVAl,
    PUBMSG,//MSGONTPC TOPIC  MSGIS MSG
    LIST
};
class Response{
    public:
        CommandTp tp;
        Response(CommandTp tp):tp(tp){};
        virtual ~Response()=default;
};
class Status:public Response{
    public:    
        int status;
        Status(int status):status(status),Response(CommandTp::STATUSVAl){}
        operator int() const {
        return status;
        }


};
class ListTpc:public Response{
   public:
     std::string list;
      ListTpc(std::string& list):list(list),Response(CommandTp::LIST){};
};
class TopicMsgCmd:public Response{
    public:
        std::string topic;
        std::string msg;
        TopicMsgCmd(const std::string& topic,const std::string& msg ):topic(topic),msg(msg),Response(CommandTp::PUBMSG){}
};
class ResponseProcessor{
    public:
        static  std::expected<std::unique_ptr<Response>,protoState::InvalidProtoMSG> getCmd(TokenList& tklist){
            int len=tklist.Tkl.size();
            auto&tklref=tklist.Tkl;
            if(len==1){
                if(tklist.Tkl[0].ttp==Tokentype::LIST){
                    return std::make_unique<ListTpc>((tklist.Tkl[0].data));
                }
            }else if(len==2){
                auto tk1=tklist.Tkl[0].ttp;
                auto tk2=tklist.Tkl[1].ttp;
                if((tk1==Tokentype::STATUS && tk2==Tokentype::STVAL) ){
                    debug::print("status type response");
                    return std::make_unique<Status>(
                        (stoi(tklist.Tkl[1].data)));
                }
                return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid comand for  length 2:{}",
                    magic_enum::enum_name(tklist.Tkl[0].ttp))));
                

            }else if(len==4){
                auto tk1=tklist.Tkl[0].ttp;
                auto tk2=tklist.Tkl[1].ttp;
                auto tk3=tklist.Tkl[2].ttp;
                auto tk4=tklist.Tkl[3].ttp;
                if(tk1==Tokentype::MSGONTPC && tk2==Tokentype::TOPIC && tk3==Tokentype::MSGIS && tk4==Tokentype::MSG){
                    return std::make_unique<TopicMsgCmd>(tklist.Tkl[1].data,tklist.Tkl[3].data);
                }
                return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid comand for  length 4:{}",
                    magic_enum::enum_name(tklist.Tkl[0].ttp))));
                
            }
            return std::unexpected(protoState::InvalidProtoMSG(std::format("invalid len of Tokens:{}",len)));
        } 
        static  std::expected<std::unique_ptr<Response>,protoState::InvalidProtoMSG> getCmd(const std::string_view resp ){
                auto tkList=TokenList::getTkList(resp);
                
                if(tkList.has_value()){
                    return getCmd(tkList.value());

                }
                return std::unexpected(protoState::InvalidProtoMSG());
        }
};
class CommandConstruct{
        
    static std::string ltrim(std::string s, const char* t =  " \t\n\r\f\v") {
        size_t start = s.find_first_not_of(t);
        if (start == std::string::npos) {
            s.clear(); 
        } else {
            s.erase(0, start);
        }
        return s;
    }

    
    static std::string rtrim(std::string s, const char* t =  " \t\n\r\f\v") {
        size_t end = s.find_last_not_of(t);
        if (end == std::string::npos) {
            s.clear(); 
        } else {
            s.erase(end + 1);
        }
        return s;
    }
    static std::string trim(std::string &&s, const char* t = " \t\n\r\f\v") {
    return ltrim(rtrim(std::move(s), t), t);
    }

    public:
        static  int getSize(std::string_view val){
        uint32_t bigEndval;
        std::memcpy(&bigEndval,val.data(),4);
        return static_cast<int>(ntohl(bigEndval));
    }
    static  std::string  getByteFromInt(int val){
        std::string charVal(4,0); 
        uint32_t bendlen=htonl(static_cast<uint32_t>(val));
        std::memcpy(charVal.data(),&bendlen,4);
        return charVal;
    }
   
     static  std::string listTopic(){
        std::string cmd="LISTTPC";
        return getByteFromInt(cmd.length())+cmd;
     }
     static std::string subTopic(std::string_view topic){
        std::string cmd="ADDTPC "+trim(std::string(topic));
        return getByteFromInt(cmd.length())+cmd;
     };
     static std::string unSubTopic(std::string_view topic){
        std::string cmd="REMTPC "+trim(std::string(topic));
        return getByteFromInt(cmd.length())+cmd;
     };
     static std::string addTopic(std::string_view topic){
        std::string cmd="ADDTPC "+trim(std::string(topic));
        return getByteFromInt(cmd.length())+cmd;
     };
     static std::string removeTopic(std::string_view topic){
        std::string cmd="REMTPC "+trim(std::string(topic));
        return getByteFromInt(cmd.length())+cmd;
     };
     static std::string publishMsg(const std::string& topic,const std::string& msg){
        std::string cmd="MSGONTPC "+topic+" MSGIS "+base64::to_base64(msg);
        return getByteFromInt(cmd.length())+cmd;
     };
     static std::string initPub(){
        std::string cmd ="PUB CREATE";
        return getByteFromInt(cmd.length())+cmd;
     }
     static std::string initSub(){
        std::string cmd ="SUB CREATE";
        return getByteFromInt(cmd.length())+cmd;
     }
};
}
#endif