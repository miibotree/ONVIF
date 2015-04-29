/*
 * =====================================================================================
 *
 *    Filename:  main.c
 *    Description:  简单例程测试:客户端通过ONVIF协议搜索前端设备
 *    Compiler:  gcc
 *    Author:  miibotree
 *
 * =====================================================================================
 */
#include "wsdd.h"
#include "wsseapi.h"
#include <stdio.h>

#define ONVIF_USER "admin"
#define ONVIF_PASSWORD ""
static struct soap* ONVIF_Initsoap(struct SOAP_ENV__Header *header, const char *was_To, const char *was_Action, int timeout);
int ONVIF_ClientDiscovery( );
int ONVIF_Capabilities(struct __wsdd__ProbeMatches *resp);
void UserGetProfiles(struct soap *soap, struct _tds__GetCapabilitiesResponse *capa_resp);
void UserGetUri(struct soap *soap, struct _trt__GetProfilesResponse *trt__GetProfilesResponse,struct _tds__GetCapabilitiesResponse *capa_resp);

int HasDev = 0;//the number of devices

int main(void )
{
	//发现协议
	if (ONVIF_ClientDiscovery() != 0 )
	{
		printf("discovery failed!\n");
		return -1;
	}
	return 0;
}


//初始化soap函数
static struct soap* ONVIF_Initsoap(struct SOAP_ENV__Header *header, const char *was_To, const char *was_Action, 
		int timeout)
{
	struct soap *soap = NULL; 
	unsigned char macaddr[6];
	char _HwId[1024];
	unsigned int Flagrand;
	soap = soap_new();
	if(soap == NULL)
	{
		printf("[%d]soap = NULL\n", __LINE__);
		return NULL;
	}
	 soap_set_namespaces( soap, namespaces);
	//超过5秒钟没有数据就退出
	if (timeout > 0)
	{
		soap->recv_timeout = timeout;
		soap->send_timeout = timeout;
		soap->connect_timeout = timeout;
	}
	else
	{
		//如果外部接口没有设备默认超时时间的话，我这里给了一个默认值10s
		soap->recv_timeout    = 10;
		soap->send_timeout    = 10;
		soap->connect_timeout = 10;
	}
	soap_default_SOAP_ENV__Header(soap, header);

	// 为了保证每次搜索的时候MessageID都是不相同的！因为简单，直接取了随机值
	srand((int)time(0));
	Flagrand = rand()%9000 + 1000; //保证四位整数
	macaddr[0] = 0x1; macaddr[1] = 0x2; macaddr[2] = 0x3; macaddr[3] = 0x4; macaddr[4] = 0x5; macaddr[5] = 0x6;
	sprintf(_HwId,"urn:uuid:%ud68a-1dd2-11b2-a105-%02X%02X%02X%02X%02X%02X", 
			Flagrand, macaddr[0], macaddr[1], macaddr[2], macaddr[3], macaddr[4], macaddr[5]);
	header->wsa__MessageID =(char *)malloc( 100);
	memset(header->wsa__MessageID, 0, 100);
	strncpy(header->wsa__MessageID, _HwId, strlen(_HwId));

	if (was_Action != NULL)
	{
		header->wsa__Action =(char *)malloc(1024);
		memset(header->wsa__Action, '\0', 1024);
		strncpy(header->wsa__Action, was_Action, 1024);//"http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
	}
	if (was_To != NULL)
	{
		header->wsa__To =(char *)malloc(1024);
		memset(header->wsa__To, '\0', 1024);
		strncpy(header->wsa__To,  was_To, 1024);//"urn:schemas-xmlsoap-org:ws:2005:04:discovery";	
	}
	soap->header = header;
	return soap;
}

int ONVIF_ClientDiscovery( )
{
	int retval = SOAP_FAULT;
	wsdd__ProbeType req;       
	struct __wsdd__ProbeMatches resp;
	wsdd__ScopesType sScope;
	struct soap *soap = NULL; 
	struct SOAP_ENV__Header header;	

	const char *was_To = "urn:schemas-xmlsoap-org:ws:2005:04:discovery";
	const char *was_Action = "http://schemas.xmlsoap.org/ws/2005/04/discovery/Probe";
	//这个就是传递过去的组播的ip地址和对应的端口发送广播信息	
	const char *soap_endpoint = "soap.udp://239.255.255.250:3702/";

	//这个接口填充一些信息并new返回一个soap对象，本来可以不用额外接口，
	// 但是后期会作其他操作，此部分剔除出来后面的操作就相对简单了,只是调用接口就好
	soap = ONVIF_Initsoap(&header, was_To, was_Action, 5);
	
	soap_default_SOAP_ENV__Header(soap, &header);
	soap->header = &header;

	soap_default_wsdd__ScopesType(soap, &sScope);
	sScope.__item = "";
	soap_default_wsdd__ProbeType(soap, &req);
	req.Scopes = &sScope;
	req.Types = "tdn:NetworkVideoTransmitter";
    
	retval = soap_send___wsdd__Probe(soap, soap_endpoint, NULL, &req);		
	//发送组播消息成功后，开始循环接收各位设备发送过来的消息
	while (retval == SOAP_OK)
    	{
		retval = soap_recv___wsdd__ProbeMatches(soap, &resp);//这个函数用来接受probe消息，存在resp里面
        if (retval == SOAP_OK) 
        {
            if (soap->error)
            {
                printf("[%d]: recv soap error :%d, %s, %s\n", __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap)); 
			    retval = soap->error;
            }
            else //成功接收某一个设备的消息
			{
				HasDev ++;
				
				if (resp.wsdd__ProbeMatches->ProbeMatch != NULL && resp.wsdd__ProbeMatches->ProbeMatch->XAddrs != NULL)
				{
					printf(" ################  recv  %d devices info #### \n", HasDev );
					
					printf("Target Service Address  : %s\n", resp.wsdd__ProbeMatches->ProbeMatch->XAddrs);	
					printf("Target EP Address       : %s\n", resp.wsdd__ProbeMatches->ProbeMatch->wsa__EndpointReference.Address);  
					printf("Target Type             : %s\n", resp.wsdd__ProbeMatches->ProbeMatch->Types);  
					printf("Target Metadata Version : %d\n", resp.wsdd__ProbeMatches->ProbeMatch->MetadataVersion); 
					ONVIF_Capabilities(&resp);//这里可以调用能力值获取函数
					sleep(1);
				}
				
			}
		}
		else if (soap->error)  
		{  
			if (HasDev == 0)
			{
				printf("[%s][%s][Line:%d] Thers Device discovery or soap error: %d, %s, %s \n",__FILE__, __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap)); 
				retval = soap->error;  
			}
			else 
			{
				printf(" [%s]-[%d] Search end! It has Searched %d devices! \n", __func__, __LINE__, HasDev);
				retval = 0;
			}
			break;
		}  
    }

    soap_destroy(soap); 
    soap_end(soap); 
    soap_free(soap);
	
	return retval;
}


int ONVIF_Capabilities(struct __wsdd__ProbeMatches *resp)
    {  
	struct _tds__GetCapabilities capa_req;
	struct _tds__GetCapabilitiesResponse capa_resp;

	struct soap *soap = NULL; 
	struct SOAP_ENV__Header header;		

        int retval = 0;   
	soap = ONVIF_Initsoap(&header, NULL, NULL, 5);
        char *soap_endpoint = (char *)malloc(256);  
        memset(soap_endpoint, '\0', 256);  
        //海康的设备，固定ip连接设备获取能力值 ,实际开发的时候，"172.18.14.22"地址以及80端口号需要填写在动态搜索到的具体信息  
        sprintf(soap_endpoint, resp->wsdd__ProbeMatches->ProbeMatch->XAddrs);  
        capa_req.Category = (enum tt__CapabilityCategory *)soap_malloc(soap, sizeof(int));  

        capa_req.__sizeCategory = 1;  
        *(capa_req.Category) = (enum tt__CapabilityCategory)0;  
        //此句也可以不要，因为在接口soap_call___tds__GetCapabilities中判断了，如果此值为NULL,则会给它赋值  
        const char *soap_action = "http://www.onvif.org/ver10/device/wsdl/GetCapabilities";  
	capa_resp.Capabilities = (struct tt__Capabilities*)soap_malloc(soap,sizeof(struct tt__Capabilities)) ;

        //这里处理鉴权函数
	soap_wsse_add_UsernameTokenDigest(soap, "user", ONVIF_USER, ONVIF_PASSWORD);
        do  
        {  
            int result = soap_call___tds__GetCapabilities(soap, soap_endpoint, soap_action, &capa_req, &capa_resp);  
            if (soap->error)  
            {  
                    printf("[%s][%d]--->>> soap error: %d, %s, %s\n", __func__, __LINE__, soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
                    retval = soap->error;  
                    break;  
            }  
            else   //获取参数成功  
            {  
                // 走到这里的时候，已经就是验证成功了，可以获取到参数了，  
                // 在实际开发的时候，可以把capa_resp结构体的那些需要的值匹配到自己的私有协议中去，简单的赋值操作就好              
                  printf("[%s][%d] Get capabilities success !\n", __func__, __LINE__);  
		if(capa_resp.Capabilities == NULL)
			printf("GetCapabilities failed! result = %d\n", result);
		else
		{
			printf(" Media->XAddr=%s \n", capa_resp.Capabilities->Media->XAddr);
			UserGetProfiles(soap, &capa_resp);//
		}
	    }
        }while(0);  
      
        free(soap_endpoint);  
        soap_endpoint = NULL;  
        soap_destroy(soap);  
        return retval;  
    }  


    void UserGetProfiles(struct soap *soap, struct _tds__GetCapabilitiesResponse *capa_resp)  
    {  
	struct _trt__GetProfiles trt__GetProfiles;
        struct _trt__GetProfilesResponse trt__GetProfilesResponse;
        int result= SOAP_OK ;  
        printf("\n-------------------Getting Onvif Devices Profiles--------------\n\n");  
        soap_wsse_add_UsernameTokenDigest(soap,"user", ONVIF_USER, ONVIF_PASSWORD);  


        result = soap_call___trt__GetProfiles(soap, capa_resp->Capabilities->Media->XAddr, NULL, &trt__GetProfiles, &trt__GetProfilesResponse);  
        if (result==-1)  
        //NOTE: it may be regular if result isn't SOAP_OK.Because some attributes aren't supported by server.  
        {  
          	 printf("soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
           	 result = soap->error;  
           	 exit(-1);  
        }  
        else{  
       		 printf("\n-------------------Profiles Get OK--------------\n\n");  
         	 if(trt__GetProfilesResponse.Profiles!=NULL)  
         	 {  
			int profile_cnt = trt__GetProfilesResponse.__sizeProfiles;
			printf("the number of profiles are: %d\n", profile_cnt);
           	     if(trt__GetProfilesResponse.Profiles->Name!=NULL) 
           	         printf("Profiles Name:%s  \n",trt__GetProfilesResponse.Profiles->Name);  

            	     if(trt__GetProfilesResponse.Profiles->token!=NULL) 
             	       printf("Profiles Taken:%s\n",trt__GetProfilesResponse.Profiles->token);  
		     UserGetUri(soap, &trt__GetProfilesResponse, capa_resp);
           	 }  
           	 else
            	    printf("Profiles Get inner Error\n");  
        } 
        printf("Profiles Get Procedure over\n");  
    }  

    void UserGetUri(struct soap *soap, struct _trt__GetProfilesResponse *trt__GetProfilesResponse,struct _tds__GetCapabilitiesResponse *capa_resp)  
    {  
	struct _trt__GetStreamUri *trt__GetStreamUri = (struct _trt__GetStreamUri *)malloc(sizeof(struct _trt__GetStreamUri));
	struct _trt__GetStreamUriResponse *trt__GetStreamUriResponse = (struct _trt__GetStreamUriResponse *)malloc(sizeof(struct _trt__GetStreamUriResponse));
        int result=0 ;  
        trt__GetStreamUri->StreamSetup = (struct tt__StreamSetup*)soap_malloc(soap,sizeof(struct tt__StreamSetup));//初始化，分配空间  
        trt__GetStreamUri->StreamSetup->Stream = 0;//stream type  
      
        trt__GetStreamUri->StreamSetup->Transport = (struct tt__Transport *)soap_malloc(soap, sizeof(struct tt__Transport));//初始化，分配空间  
        trt__GetStreamUri->StreamSetup->Transport->Protocol = 0;  
        trt__GetStreamUri->StreamSetup->Transport->Tunnel = 0;  
        trt__GetStreamUri->StreamSetup->__size = 1;  
        trt__GetStreamUri->StreamSetup->__any = NULL;  
        trt__GetStreamUri->StreamSetup->__anyAttribute =NULL;  
      
      
        trt__GetStreamUri->ProfileToken = trt__GetProfilesResponse->Profiles->token ;  
      
        printf("\n\n---------------Getting Uri----------------\n\n");  
      
        soap_wsse_add_UsernameTokenDigest(soap,"user", ONVIF_USER, ONVIF_PASSWORD);  
        soap_call___trt__GetStreamUri(soap, capa_resp->Capabilities->Media->XAddr, NULL, trt__GetStreamUri, trt__GetStreamUriResponse);  
      
      
        if (soap->error) {  
        printf("soap error: %d, %s, %s\n", soap->error, *soap_faultcode(soap), *soap_faultstring(soap));  
        result = soap->error;  
      
        }  
        else{  
            printf("!!!!NOTE: RTSP Addr Get Done is :%s \n",trt__GetStreamUriResponse->MediaUri->Uri);  
	    //最后我们使用vlc -vvv命令来打开一个rstp网络流
	    char cmd_str[256];
	    memset(cmd_str, 0, sizeof(cmd_str));
	    sprintf(cmd_str, "vlc %s --sout=file/ps:device%d.mp4", trt__GetStreamUriResponse->MediaUri->Uri, HasDev);
	    popen((const char *)cmd_str, "r");		
        }  
    }  

