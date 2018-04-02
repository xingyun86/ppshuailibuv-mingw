
/***************************************

* @file uvsocket.h

* @brief ����libuv��װ��tcp��������ͻ���,ʹ��log4z����־����

* @details

* @author ppshuai

* @date 2016-12-26

* ������������ͻ��˵Ĵ���.�ַ�����֧�ֶ�ͻ�������

�޸Ŀͻ��˲��Դ��룬֧�ֲ�����ͻ��˲���

****************************************/

#ifndef __UVSOCKET_H
#define __UVSOCKET_H

#include <stdlib.h>
#include <iostream>
#include <string>
#include <stdstring.h>
#include <list>
#include <map>
#include <log4z.h>
#include <uv.h>

#define BUFFERSIZE (1024*1024)

namespace uvsocket
{
    typedef void (*newconnect)(int clientid);
    typedef void (*server_recvcb)(int cliendid, const char* buf, int bufsize);
    typedef void (*client_recvcb)(const char* buf, int bufsize, void* userdata);

    class CTCPServer;
    class CSockData
    {
    public:

        CSockData(int clientid):client_id(clientid),recvcb_(0/*nullptr*/) {

            client_handle = (uv_tcp_t*)malloc(sizeof(*client_handle));
            client_handle->data = this;
            readbuffer = uv_buf_init((char*)malloc(BUFFERSIZE), BUFFERSIZE);
            writebuffer = uv_buf_init((char*)malloc(BUFFERSIZE), BUFFERSIZE);
        }

        virtual ~CSockData() {
            free(readbuffer.base);
            readbuffer.base = 0/*nullptr*/;
            readbuffer.len = 0;
            free(writebuffer.base);
            writebuffer.base = 0/*nullptr*/;
            writebuffer.len = 0;
            free(client_handle);
            client_handle = 0/*nullptr*/;
        }

        int client_id;//�ͻ���id,Ωһ
        uv_tcp_t* client_handle;//�ͻ��˾��
        CTCPServer* tcp_server;//���������(��������ΪĳЩ�ص�������Ҫ��)
        uv_buf_t readbuffer;//�������ݵ�buf
        uv_buf_t writebuffer;//д���ݵ�buf
        uv_write_t write_req;
        server_recvcb recvcb_;//�������ݻص����û��ĺ���
    };
    typedef std::map<int,CSockData*>    INTSOCKDATAMAP;
    typedef INTSOCKDATAMAP::iterator    INTSOCKDATAMAPIT;
    typedef INTSOCKDATAMAP::value_type  INTSOCKDATAMAPPAIR;
    class CTCPServer
    {
    public:

        CTCPServer(uv_loop_t* loop = uv_default_loop());
        virtual ~CTCPServer();
        static void StartLog(const char* logpath = 0);//������־�����������Ż�������־

    public:
        //��������
        bool Start(const char *ip, int port);//����������,��ַΪIP4
        bool Start6(const char *ip, int port);//��������������ַΪIP6
        void close();
        bool setNoDelay(bool enable);
        bool setKeepAlive(int enable, unsigned int delay);
        const char* GetLastErrMsg() const {
            return errmsg_.c_str();
        };

        virtual int send(int clientid, const char* data, std::size_t len);
        virtual void setnewconnectcb(newconnect cb);
        virtual void setrecvcb(int clientid,server_recvcb cb);//���ý��ջص�������ÿ���ͻ��˸���һ��

    protected:
        int GetAvailalClientID()const;//��ȡ���õ�client id
        bool DeleteClient(int clientid);//ɾ�������еĿͻ���
        //��̬�ص�����
        static void AfterServerRecv(uv_stream_t *client, ssize_t nread, const uv_buf_t* buf);
        static void AfterSend(uv_write_t *req, int status);
        static void onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
        static void AfterServerClose(uv_handle_t *handle);
        static void AfterClientClose(uv_handle_t *handle);
        static void acceptConnection(uv_stream_t *server, int status);

    private:
        bool init();
        bool run(int status = UV_RUN_DEFAULT);
        bool bind(const char* ip, int port);
        bool bind6(const char* ip, int port);
        bool listen(int backlog = 1024);
        uv_tcp_t server_;//����������
        INTSOCKDATAMAP clients_list_;//�ӿͻ�������
        uv_mutex_t mutex_handle_;//����clients_list_
        uv_loop_t *loop_;
        std::string errmsg_;
        newconnect newconcb_;
        bool isinit_;//�Ƿ��ѳ�ʼ��������close�������ж�
    };

    class CTCPClient
    {
    //ֱ�ӵ���connect/connect6���������
    public:
        CTCPClient(uv_loop_t* loop = uv_default_loop());
        virtual ~CTCPClient();
        static void StartLog(const char* logpath = 0/*nullptr*/);//������־�����������Ż�������־
    public:
        //��������
        virtual bool connect(const char* ip, int port);//����connect�̣߳�ѭ���ȴ�ֱ��connect���
        virtual bool connect6(const char* ip, int port);//����connect�̣߳�ѭ���ȴ�ֱ��connect���
        virtual int send(const char* data, std::size_t len);
        virtual void setrecvcb(client_recvcb cb, void* userdata);////���ý��ջص�������ֻ��һ��
        void close();
        //�Ƿ�����Nagle�㷨
        bool setNoDelay(bool enable);
        bool setKeepAlive(int enable, unsigned int delay);
        const char* GetLastErrMsg() const {
            return errmsg_.c_str();
        };

    protected:
        //��̬�ص�����
        static void AfterConnect(uv_connect_t* handle, int status);
        static void AfterClientRecv(uv_stream_t *client, ssize_t nread, const uv_buf_t* buf);
        static void AfterSend(uv_write_t *req, int status);
        static void onAllocBuffer(uv_handle_t *handle, size_t suggested_size, uv_buf_t *buf);
        static void AfterClose(uv_handle_t *handle);
        static void ConnectThread(void* arg);//������connect�߳�
        static void ConnectThread6(void* arg);//������connect�߳�
        bool init();
        bool run(int status = UV_RUN_DEFAULT);
    private:
        enum {
            CONNECT_TIMEOUT,
            CONNECT_FINISH,
            CONNECT_ERROR,
            CONNECT_DIS,
        };
        uv_tcp_t client_;//�ͻ�������
        uv_loop_t *loop_;
        uv_write_t write_req_;//дʱ����
        uv_connect_t connect_req_;//����ʱ����
        uv_thread_t connect_threadhanlde_;//�߳̾��
        std::string errmsg_;//������Ϣ
        uv_buf_t readbuffer_;//�������ݵ�buf
        uv_buf_t writebuffer_;//д���ݵ�buf
        uv_mutex_t write_mutex_handle_;//����write,����ǰһwrite��ɲŽ�����һwrite
        int connectstatus_;//����״̬
        client_recvcb recvcb_;//�ص�����
        void* userdata_;//�ص��������û�����
        std::string connectip_;//���ӵķ�����IP
        int connectport_;//���ӵķ������˿ں�
        bool isinit_;//�Ƿ��ѳ�ʼ��������close�������ж�
    };
}

#endif // __UVSOCKET_H
