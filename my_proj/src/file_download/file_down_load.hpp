#include "httpSocket.h"

int DownLoadFile(std::string serverName, std::string URL,
        std::string localDirectory)
{
    FILE *fp;
    if (!(fp = fopen(localDirectory.c_str(), "wb+")))
    {
        printf("can't open local file\n");
        return -1;
    }

    CHttpSocket *cs = new CHttpSocket();
    cs->Socket();
    cs->Connect(serverName.c_str(), 80);

    long len = 0;
    std::string req = cs->FormatRequestHeader(serverName.c_str(), URL.c_str(),
            len, NULL, NULL, 0, 0, 0);
    cs->SendRequest(req.c_str(), len);

    int lens;
    std::string head = cs->GetResponseHeader(lens);

    printf("%s\n", head.c_str());

    int cnt = 0;
    int flag = head.find("Content-Length:", 0);
    int endFlag = head.find("\r\n", flag);
    std::string subStr = head.substr(flag, endFlag - flag);

    sscanf(subStr.c_str(), "Content-Length: %d", &lens);

    fseek(fp, 0, 0);

    while (cnt < lens)
    {
        char buff[1025];
        int tmplen = cs->Receive(buff, 1024);
        cnt += tmplen;
        fwrite(buff, 1, tmplen, fp);
    }

    fclose(fp);
    return 0;
}
