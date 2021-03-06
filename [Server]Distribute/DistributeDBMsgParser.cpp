#include "stdafx.h"
#include "DistributeDBMsgParser.h"
#include "DataBase.h"
#include "CommonDBMsgParser.h"
#include "UserTable.h"
#include "ServerTable.h"
#include "Network.h"
#include "UserManager.h"

extern int g_nServerSetNum;
extern DWORD g_dwMaxUser;

DBMsgFunc g_DBMsgFunc[] =
{
	NULL,
	RLoginCheckQuery,
	RIpCheckQuery,			/// eIpCheckQuery 
	RLoginCheckQueryTest,
	RCheckDynamicPW,
	RLoginCheckQueryForJP,
	NULL, //eHackToolUser
};

void LoginCheckInit()
{
	g_DB.FreeQuery(0, 0, "EXEC %s", "MP_CHARACTER_LoginInit");
/* ¾ÆÁ÷ Ãß°¡ÇÏÁö ¸»°Í... ÇÁ·Î½ÃÁ® ¼öÁ¤ÈÄ Ãß°¡ (LBS)
	char txt[128];
	sprintf(txt, "EXEC %s %d", "up_Server_ResetLoginMember", g_nServerSetNum);
	g_DB.LoginQuery(eQueryType_FreeQuery, 0, 0, txt);
*/
}


/*
void LoginCheckDelete(DWORD UserID)//, DWORD dwConnectionIndex)
{
	char txt[128];
	sprintf(txt, "EXEC %s %d", "Up_Member_CheckOut", UserID);
	g_DB.LoginQuery(eQueryType_FreeQuery, eNull, 0, txt);
}


BOOL LoginCheckQuery(char* id, char* pw, DWORD AuthKey, DWORD dwConnectionIndex)
{
	char guery[128];
	// id,pw,AuthKey,ServerGroupNum
	sprintf(guery, "EXEC %s \'%s\', \'%s\', %d, %d", "Up_Member_CheckIn", id, pw, AuthKey, 1);
	if(g_DB.LoginQuery(eQueryType_FreeQuery, eLoginCheckQuery, dwConnectionIndex, guery) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy  ID:%d  ConnectionIndex:%d",id,dwConnectionIndex);
		return FALSE;
	}
	
	g_Console.Log(eLogDisplay,4,"IDPW_QUERY   ID: %s,  ConnectionIndex: %d",id,dwConnectionIndex);
	return TRUE;
}
*/

BOOL IPCheckQuery(char* ip, DWORD AuthKey, DWORD dwConnectionIndex)
{
	char query[128];
	sprintf(query, "EXEC %s \'%s\', %d", "Up_Ip_CheckIn", ip, AuthKey);
	if(g_DB.LoginQuery(eQueryType_FreeQuery, eIpCheckQuery, dwConnectionIndex, query) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy(IP_CHECK)  IP:%s  ConnectionIndex:%d", ip, dwConnectionIndex);
		return FALSE;
	}

//	g_Console.Log(eLogDisplay,4,"IP_QUERY   IP: %s,  ConnectionIndex: %d", ip, dwConnectionIndex);
	return TRUE;
}

/// 060911 PKH ȫ��IPüũ
/* --------------------------------------------------------------------------
  > * �Լ��̸� : IPCheckQuery
  > * ��    �� : ȫ��IPüũ
  > * ���ǻ��� :
  > *---------------------------------------------------------------------------*/
BOOL IPCheckQueryHK(char* ip, DWORD AuthKey, DWORD dwConnectionIndex)
{
	char query[128];
	sprintf(query, "EXEC %s \'%s\', %d", "Up_Ip_CheckInHK", ip, AuthKey);
	if(g_DB.LoginQuery(eQueryType_FreeQuery, eIpCheckQuery, dwConnectionIndex, query) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy(IP_CHECK)  IP:%s  ConnectionIndex:%d", ip, dwConnectionIndex);
		return FALSE;
	}
	return TRUE;
}

void LoginError(DWORD dwConnectionIdx, DWORD AuthKey,DWORD ErrorCode,DWORD dwParam)
{	
	MSG_DWORD2 msguser;
	msguser.Category = MP_USERCONN;
	msguser.Protocol = MP_USERCONN_LOGIN_NACK;
	msguser.dwData1 = ErrorCode;
	msguser.dwData2 = dwParam;
	
	g_pUserTable->SendToUser(dwConnectionIdx, AuthKey, &msguser, sizeof(msguser) );

	
//	g_Console.Log(eLogDisplay,4,"  LOGINERROR   Errorcode: %d,  ConnectionIndex: %d",ErrorCode,dwConnectionIdx);
}

void RIpCheckQuery(LPQUERY pData, LPDBMESSAGE pMessage)
{
	if(pMessage->dwResult == 0)
	{
		// DB Error(DB is busy)
		return;
	}

	WORD Ecode =atoi((char*)pData->Data[0]);
	DWORD AuthKey = atoi((char*)pData->Data[1]);

	DWORD dwConnectionIdx	= pMessage->dwID;
	USERINFO* pUserInfo		= g_pUserTable->FindUser(dwConnectionIdx);

	if( pUserInfo == NULL )
		return;

	if( pUserInfo->dwUniqueConnectIdx != AuthKey )
		return;

	if( Ecode == 0 )
	{
		LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_INVALID_IP );
        DisconnectUser( dwConnectionIdx );
		return;
	}


	SERVERINFO* FastInfo = g_pServerTable->GetFastServer( AGENT_SERVER );
	
	if(FastInfo == NULL)
	{
		LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOAGENTSERVER );
        DisconnectUser( dwConnectionIdx );
		return;
	}

	MSG_DWORD3BYTE2 msg;
	msg.Category	= MP_USERCONN;
	msg.Protocol	= MP_USERCONN_NOTIFY_USERLOGIN_SYN;
	msg.dwObjectID	= pUserInfo->dwUserID;
	msg.dwData1		= AuthKey;
	msg.dwData2		= dwConnectionIdx;
	msg.dwData3		= 0;
	msg.bData1		= pUserInfo->UserLevel;
	msg.bData2		= 0;
				
	g_Network.Send2Server(FastInfo->dwConnectionIndex,(char*)&msg,sizeof(msg));
}

DWORD testTime[1024];
BOOL LoginCheckQueryTest(char* id, char* pw, DWORD AuthKey, DWORD dwConnectionIndex)
{
	char guery[128];
	sprintf(guery, "EXEC %s \'%s\', \'%s\', %d, %d", "Up_Member_CheckIn", id, pw, AuthKey, 1);
	if(g_DB.LoginQuery(101, eLoginCheckQueryTest, dwConnectionIndex, guery) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy  ID:%d  ConnectionIndex:%d",id,dwConnectionIndex);
		return FALSE;
	}
	
	g_Console.Log(eLogDisplay,4,"IDPW_QUERY   ID: %s,  ConnectionIndex: %d",id,dwConnectionIndex);
	return TRUE;
}

void	RLoginCheckQueryTest(LPQUERY pData, LPDBMESSAGE pMessage)
{
	DWORD AuthKey =atoi((char*)pData->Data[2]);
	
	testTime[AuthKey-1] = GetTickCount() - pMessage->dwID;

	g_Console.Log(eLogDisplay,4,"IDPW TestReceived    ID: %d  Time: %d",AuthKey,testTime[AuthKey-1]);

	if(AuthKey == 1024)
	{
		FILE* fp = fopen("test.txt","w");
		for(int n=0;n<1024;++n)
			fprintf(fp,"%d  ElapsedTime: %d\n",n+1,testTime[n]);
		fclose(fp);
	}
}

//* MemberDB ±³Ã¼ ?Û¾÷ 2004.03.22
void LoginCheckDelete(DWORD UserID)
{
	char txt[128];
#ifdef _JAPAN_LOCAL_
	sprintf(txt, "EXEC %s %d", "Up_GameLogout_JP", UserID);
#else
	sprintf(txt, "EXEC %s %d", "Up_GameLogout", UserID);
#endif
	g_DB.LoginQuery(eQueryType_FreeQuery, eNull, 0, txt);
}
//*/

//* MemberDB ±³Ã¼ ?Û¾÷ 2004.03.22
BOOL LoginCheckQuery(char* id, char* pw, char* ip, WORD ServerNo, DWORD AuthKey, DWORD dwConnectionIndex, DWORD dwMaxUser, WORD ServerNum)
{
	char guery[128];
	// id,pw,AuthKey,ServerGroupNum
	// MaxUser°Ë»çÇÒ¶§
	
	sprintf(guery, "EXEC %s \'%s\', \'%s\', \'%s\', %d, \'%d\', %d, %d, %d", 
						"up_gameloginadult_free", id, pw, ip, ServerNo, AuthKey, dwMaxUser, gUserMGR.GetLimitAge(), ServerNum);

	if(g_DB.LoginQuery(eQueryType_FreeQuery, eLoginCheckQuery, dwConnectionIndex, guery, FALSE) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy  ID:%d  ConnectionIndex:%d",id,dwConnectionIndex);
		LoginError( dwConnectionIndex, AuthKey, LOGIN_ERROR_DISTSERVERISBUSY );
		return FALSE;
	}

	return TRUE;
}

#ifdef _KOR_LOCAL_
BOOL LoginCheckQueryforMD5(char* id, char* pw, char* ip, WORD ServerNo, DWORD AuthKey, DWORD dwConnectionIndex, DWORD dwMaxUser, WORD ServerNum, char* pPN)
{
	//SW080401 ����ũ���߰�
	char guery[256];

	// id,pw,AuthKey,ServerGroupNum
	// MaxUser°Ë»çÇÒ¶§
	/*
	sprintf(guery, "EXEC %s \'%s\', \'%s\', \'%s\', %d, \'%d\', %d, %d, %d", 
		"up_gameloginadult_free", id, pw, ip, ServerNo, AuthKey, dwMaxUser, gUserMGR.GetLimitAge(), ServerNum);
		*/
	
	sprintf(guery, "EXEC %s \'%s\', \'%s\', \'%s\', %d, \'%d\', %d, %d, %d, \'%s\'", 
		"up_GameLoginAdult_FREE_ver1", id, pw, ip, ServerNo, AuthKey, dwMaxUser, gUserMGR.GetLimitAge(), ServerNum, pPN);
	
	if(g_DB.LoginMiddleQuery( RLoginCheckQueryforMD5, dwConnectionIndex, guery ) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy  ID:%d  ConnectionIndex:%d",id,dwConnectionIndex);
		LoginError( dwConnectionIndex, AuthKey, LOGIN_ERROR_DISTSERVERISBUSY );
		return FALSE;
	}

	return TRUE;
}

//2008. 3. 18. CBH - ���ȹ�ȣ�� ���� �߰�
BOOL LoginCheckQueryEx(char* id, char* pw, char* ip, WORD ServerNo, DWORD AuthKey, DWORD dwConnectionIndex, DWORD dwMaxUser, WORD ServerNum, char* pPN)
{
	//SW080401 ����ũ���߰�
	char guery[256];

	sprintf(guery, "EXEC %s \'%s\', \'%s\', \'%s\', %d, \'%d\', %d, %d, %d, \'%s\'", 
		"up_GameLoginAdult_FREE_ver1", id, pw, ip, ServerNo, AuthKey, dwMaxUser, gUserMGR.GetLimitAge(), ServerNum, pPN);

	if(g_DB.LoginQuery(eQueryType_FreeQuery, eLoginCheckQuery, dwConnectionIndex, guery, FALSE) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy  ID:%d  ConnectionIndex:%d",id,dwConnectionIndex);
		LoginError( dwConnectionIndex, AuthKey, LOGIN_ERROR_DISTSERVERISBUSY );
		return FALSE;
	}

	return TRUE;
}
#endif
//*/



//* MemberDB ±³Ã¼ ?Û¾÷ 2004.03.22
void RLoginCheckQuery(LPQUERY pData, LPDBMESSAGE pMessage)
{
	DWORD	stateNo = atoi((char*)pData->Data[0]);
	WORD	ServerNo = atoi((char*)pData->Data[1]);
	DWORD	UserIdx = atoi((char*)pData->Data[2]);
	WORD	wType = atoi((char*)pData->Data[3]);
	int		nTime = atoi((char*)pData->Data[4]);
//	WORD	bTen = atoi((char*)pData->Data[5]);
	DWORD	AuthKey = atoi((char*)pData->Data[6]);
	BYTE	UserLevel = atoi((char*)pData->Data[7]);
	WORD	AgentNo = atoi((char*)pData->Data[8]);
	DWORD	dwConnectionIdx = pMessage->dwID;


	USERINFO* pUserInfo = g_pUserTable->FindUser(dwConnectionIdx);

	if(pUserInfo == NULL)					//������ ������.
	{
		//���� �������� ���� �α��� delete�� ���ش�.
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	if(pUserInfo->dwUniqueConnectIdx != AuthKey)	//�������� ���� �����, �ٸ� ������ ���Դ�.
	{												//***pUserInfo�� ������ �ƴϴ�. ����� �ȵȴ�.***
		//���� �������� ���� �α��� delete�� ���ش�.		
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	pUserInfo->dwUserID		= UserIdx;
 	pUserInfo->UserLevel	= UserLevel;
	pUserInfo->State		= stateNo;

	switch(stateNo)
	{
	case 1:	//�α��� ����
		{
			pUserInfo->dwDisconOption |= eDISCONOPT_DELETELOGINCHECK;	//AAA
			//�������� ����� DeleteLoginCheck�� ���ְ� �ȴ�. 
			//�����κп��� ȣ���ϴ� ���� OndisconnectUser���� ȣ���ϵ��� �����Ѵ�.

			g_LoginUser.Remove( UserIdx );

			if(gUserMGR.IsValidUserLevel(UserLevel) == FALSE)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_INVALIDUSERLEVEL);
				DisconnectUser(dwConnectionIdx);
				return;
			}

//china payment
#ifdef _CHINA_LOCAL_
			if( wType == 8 )
			{
				if( nTime <= 0 )
				{
					LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_NOREMAINTIME);
					DisconnectUser(dwConnectionIdx);
					return;
				}
			}
#endif
//////			

/// 060911 PKH ȫ��IPüũ
#ifdef _HK_LOCAL_
			if( UserLevel <= eUSERLEVEL_GM )
			{
				char strIP[16] = {0,};
				WORD wPort;
				g_Network.GetUserAddress( dwConnectionIdx, strIP, &wPort );

				IPCheckQueryHK( strIP, AuthKey, dwConnectionIdx );
				return;
			}
#else
			if( UserLevel == eUSERLEVEL_GM )
			{
				char strIP[16] = {0,};
				WORD wPort;
				g_Network.GetUserAddress( dwConnectionIdx, strIP, &wPort );

				IPCheckQuery( strIP, AuthKey, dwConnectionIdx );
				return;
			}
#endif

			SERVERINFO* pAgentServer = g_pServerTable->GetServer(AGENT_SERVER, AgentNo);
			if(pAgentServer == NULL)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_NOAGENTSERVER);
				DisconnectUser( dwConnectionIdx );
				return;
			}

			MSG_DWORD3BYTE2 msg;
			msg.Category	= MP_USERCONN;
			msg.Protocol	= MP_USERCONN_NOTIFY_USERLOGIN_SYN;
			msg.dwObjectID	= UserIdx;
			msg.dwData1		= AuthKey;
			msg.dwData2		= dwConnectionIdx;
			msg.dwData3		= (DWORD)nTime;
			msg.bData1		= (BYTE)UserLevel;
			msg.bData2		= (BYTE)wType;
			
			g_Network.Send2Server(pAgentServer->dwConnectionIndex,(char*)&msg,sizeof(msg));
				
			//g_pLoginUser.Remove( pUserInfo->dwUserID );
		}
		break;
	case 2: // ������� ����
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW, 2);
		}
		break;
	case 3: // ������� ����
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);			
		}
		break;
	case 4: // �ߺ��α���
		{
			if(g_nServerSetNum == ServerNo)
			{
				MSG_DWORD msg;
				msg.Category	= MP_USERCONN;
				msg.Protocol	= MP_USERCONN_NOTIFY_OVERLAPPEDLOGIN;
				msg.dwData		= UserIdx;
				g_Network.Broadcast2AgentServer((char*)&msg,sizeof(msg));
			
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGIN,0);
			}
			else
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGININOTHERSERVER,ServerNo);
			}
		}
		break;

	case 5: //����
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_BLOCKUSERLEVEL);
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;

	case 7: // DB Connection Error
		{
			ASSERTMSG(0, "login check error ret 0");
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;

	case 8: // ? �ƽ�����?
		{			
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MAXUSER );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
		//---PUNISH
	case 11: // �������� ����
		{
			LoginError(dwConnectionIdx, AuthKey, LOGIN_ERROR_BLOCKUSERLEVEL, nTime);
			DisconnectUser(dwConnectionIdx);
		}
		break;
		//-------------
	case 14:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOT_CLOSEBETAWINNER );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	case 15: //�̼�����
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MINOR_INADULTSERVER );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	case 16:	//���ȹ�ȣ ����
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_PROTECTIONNUMBER );			
		}
		break;
	case 17:	//���ȹ�ȣ ����
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_PROTECTIONNUMBER_REGIST );			
		}
		break;
	case 50: // Ż������
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_SECEDEDACCOUNT);
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	case 99:		
		{
			LoginGetDynamicPWQuery( (char*)pData->Data[8], dwConnectionIdx );		
		}
		break;
	case 999:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOIPREGEN );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	default:
		{
			ASSERT(0);
		}
		break;
	}
}


#ifdef _KOR_LOCAL_
void RLoginCheckQueryforMD5(LPMIDDLEQUERY pData, LPDBMESSAGE pMessage)
{
	DWORD	stateNo = atoi((char*)pData->Data[0]);
	WORD	ServerNo = atoi((char*)pData->Data[1]);
	DWORD	UserIdx = atoi((char*)pData->Data[2]);
	WORD	wType = atoi((char*)pData->Data[3]);
	int		nTime = atoi((char*)pData->Data[4]);
	//	WORD	bTen = atoi((char*)pData->Data[5]);
	DWORD	AuthKey = atoi((char*)pData->Data[6]);
	BYTE	UserLevel = atoi((char*)pData->Data[7]);
	WORD	AgentNo = atoi((char*)pData->Data[8]);
	DWORD	dwConnectionIdx = pMessage->dwID;


	USERINFO* pUserInfo = g_pUserTable->FindUser(dwConnectionIdx);

	if(pUserInfo == NULL)					//������ ������.
	{
		//���� �������� ���� �α��� delete�� ���ش�.
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	if(pUserInfo->dwUniqueConnectIdx != AuthKey)	//�������� ���� �����, �ٸ� ������ ���Դ�.
	{												//***pUserInfo�� ������ �ƴϴ�. ����� �ȵȴ�.***
		//���� �������� ���� �α��� delete�� ���ش�.		
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	pUserInfo->dwUserID		= UserIdx;
	pUserInfo->UserLevel	= UserLevel;
	pUserInfo->State		= stateNo;

	switch(stateNo)
	{
	case 1:	//�α��� ����
		{
			pUserInfo->dwDisconOption |= eDISCONOPT_DELETELOGINCHECK;	//AAA
			//�������� ����� DeleteLoginCheck�� ���ְ� �ȴ�. 
			//�����κп��� ȣ���ϴ� ���� OndisconnectUser���� ȣ���ϵ��� �����Ѵ�.

			g_LoginUser.Remove( UserIdx );

			if(gUserMGR.IsValidUserLevel(UserLevel) == FALSE)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_INVALIDUSERLEVEL);
				DisconnectUser(dwConnectionIdx);
				return;
			}

			if( UserLevel == eUSERLEVEL_GM )
			{
				char strIP[16] = {0,};
				WORD wPort;
				g_Network.GetUserAddress( dwConnectionIdx, strIP, &wPort );

				IPCheckQuery( strIP, AuthKey, dwConnectionIdx );
				return;
			}

			SERVERINFO* pAgentServer = g_pServerTable->GetServer(AGENT_SERVER, AgentNo);
			if(pAgentServer == NULL)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_NOAGENTSERVER);
				DisconnectUser( dwConnectionIdx );
				return;
			}

			MSG_DWORD3BYTE2 msg;
			msg.Category	= MP_USERCONN;
			msg.Protocol	= MP_USERCONN_NOTIFY_USERLOGIN_SYN;
			msg.dwObjectID	= UserIdx;
			msg.dwData1		= AuthKey;
			msg.dwData2		= dwConnectionIdx;
			msg.dwData3		= (DWORD)nTime;
			msg.bData1		= (BYTE)UserLevel;
			msg.bData2		= (BYTE)wType;

			g_Network.Send2Server(pAgentServer->dwConnectionIndex,(char*)&msg,sizeof(msg));

			//g_pLoginUser.Remove( pUserInfo->dwUserID );
		}
		break;
	case 2: // ������� ����
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);
		}
		break;
	case 3: // ������� ����
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);			
		}
		break;
	case 4: // �ߺ��α���
		{
			if(g_nServerSetNum == ServerNo)
			{
				MSG_DWORD msg;
				msg.Category	= MP_USERCONN;
				msg.Protocol	= MP_USERCONN_NOTIFY_OVERLAPPEDLOGIN;
				msg.dwData		= UserIdx;
				g_Network.Broadcast2AgentServer((char*)&msg,sizeof(msg));

				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGIN,0);
			}
			else
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGININOTHERSERVER,ServerNo);
			}
		}
		break;

	case 5: //����
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_BLOCKUSERLEVEL);
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;

	case 7: // DB Connection Error
		{
			ASSERTMSG(0, "login check error ret 0");
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;

	case 8: // ? �ƽ�����?
		{			
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MAXUSER );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
		//---PUNISH
	case 11: // �������� ����
		{
			LoginError(dwConnectionIdx, AuthKey, LOGIN_ERROR_BLOCKUSERLEVEL, nTime) ;
			DisconnectUser(dwConnectionIdx);
		}
		break;
		//-------------
	case 14:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOT_CLOSEBETAWINNER );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	case 15: //�̼�����
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MINOR_INADULTSERVER );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	case 16:	//���ȹ�ȣ ����
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_PROTECTIONNUMBER );			
		}
		break;
	case 17:	//���ȹ�ȣ ����
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_PROTECTIONNUMBER_REGIST );			
		}
		break;
	case 50: // Ż������
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_SECEDEDACCOUNT);
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	case 99:		
		{
			LoginGetDynamicPWQuery( (char*)pData->Data[8], dwConnectionIdx );		
		}
		break;
	case 999:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOIPREGEN );
			DisconnectUser(dwConnectionIdx);	//AAA ������ ���� ���� ����.
		}
		break;
	default:
		{
			ASSERT(0);
		}
		break;
	}
}
#endif
//*/

BOOL LoginGetDynamicPWQuery( char* id, DWORD dwConnectionIndex )
{
//TW mosong's game_code=15;
//HK mosong's game_code=11;
	char guery[128];
#ifdef _HK_LOCAL_
#ifdef _TW_LOCAL_
	sprintf( guery, "EXEC %s \'%s\', 15, '', 1", "lp_mat_out", id );
#else
	sprintf( guery, "EXEC %s \'%s\', 11, '', 1", "lp_mat_out", id );
#endif
#else
	sprintf( guery, "EXEC %s \'%s\', '', 1", "lp_mat_out", id );
#endif

	if(g_DB.LoginMiddleQuery( RLoginGetDynamicPWQuery, dwConnectionIndex, guery) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

void RLoginGetDynamicPWQuery(LPMIDDLEQUERY pData, LPDBMESSAGE pMessage)
{
	DWORD dwConnectionIdx = pMessage->dwID;
	int rt = atoi((char*)pData->Data[0]);

	if( rt == 1 )
	{
		//send mat msg
		MSG_USE_DYNAMIC_ACK msg;
		msg.Category = MP_USERCONN;
		msg.Protocol = MP_USERCONN_USE_DYNAMIC_ACK;
		SafeStrCpy( msg.mat, (char*)pData->Data[1], 32 );

		g_Network.Send2User( dwConnectionIdx, (char*)&msg, sizeof(msg) );			
	}
	else
	{
		//error msg
		MSGBASE msg;
		msg.Category = MP_USERCONN;
		msg.Protocol = MP_USERCONN_USE_DYNAMIC_NACK;
		g_Network.Send2User( dwConnectionIdx, (char*)&msg, sizeof(msg) );
	}
}

BOOL CheckDynamicPW( char* id, char* pw, char* ip, WORD ServerNo, DWORD AuthKey, DWORD dwConnectionIndex, DWORD dwMaxUser, WORD ServerNum )
{
	char guery[128];
	sprintf(guery, "EXEC up_gameloginmat_free \'%s\', \'%s\', \'%s\', %d, \'%d\', %d, %d, %d",
		id, pw, ip, ServerNo, AuthKey, dwMaxUser, gUserMGR.GetLimitAge(), ServerNum );
	if(g_DB.LoginQuery(eQueryType_FreeQuery, eCheckDynamicPW, dwConnectionIndex, guery, FALSE) == FALSE)
		return FALSE;

	return TRUE;
}

void RCheckDynamicPW( LPQUERY pData, LPDBMESSAGE pMessage )
{
	DWORD stateNo = atoi((char*)pData->Data[0]);
	WORD ServerNo = atoi((char*)pData->Data[1]);
	DWORD UserIdx = atoi((char*)pData->Data[2]);
	WORD	wType = atoi((char*)pData->Data[3]);
	int		nTime = atoi((char*)pData->Data[4]);
//	WORD bTen = atoi((char*)pData->Data[5]);
	DWORD AuthKey = atoi((char*)pData->Data[6]);
	BYTE UserLevel = atoi((char*)pData->Data[7]);
	WORD AgentNo = atoi((char*)pData->Data[8]);
	DWORD dwConnectionIdx = pMessage->dwID;

	USERINFO* pUserInfo = g_pUserTable->FindUser(dwConnectionIdx);

	if(pUserInfo == NULL)					//������ ������.
	{	
		//���� �������� ���� �α��� delete�� ���ش�.
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	if(pUserInfo->dwUniqueConnectIdx != AuthKey)
	{
		//���� �������� ���� �α��� delete�� ���ش�.		
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	pUserInfo->dwUserID		= UserIdx;
	pUserInfo->UserLevel	= UserLevel;
	pUserInfo->State		= stateNo;

	switch(stateNo)
	{
	case 1:
		{
			pUserInfo->dwDisconOption |= eDISCONOPT_DELETELOGINCHECK;	//AAA
			//�������� ����� DeleteLoginCheck�� ���ְ� �ȴ�. 
			//�����κп��� ȣ���ϴ� ���� OndisconnectUser���� ȣ���ϵ��� �����Ѵ�.

			g_LoginUser.Remove( UserIdx );

			if(gUserMGR.IsValidUserLevel(UserLevel) == FALSE)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_INVALIDUSERLEVEL);
				DisconnectUser(dwConnectionIdx);
				return;
			}
			
/// 060921 PKH ȫ��븸 ���̳���IPüũ
#ifdef _HK_LOCAL_
			if( UserLevel <= eUSERLEVEL_GM )
			{
				char strIP[16] = {0,};
				WORD wPort;
				g_Network.GetUserAddress( dwConnectionIdx, strIP, &wPort );

				IPCheckQueryHK( strIP, AuthKey, dwConnectionIdx );
				return;
			}
#else
			if( UserLevel == eUSERLEVEL_GM )
			{
				char strIP[16] = {0,};
				WORD wPort;
				g_Network.GetUserAddress( dwConnectionIdx, strIP, &wPort );

				IPCheckQuery( strIP, AuthKey, dwConnectionIdx );
				return;
			}
#endif


			SERVERINFO* pAgentServer = g_pServerTable->GetServer(AGENT_SERVER, AgentNo);
			if(pAgentServer == NULL)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_NOAGENTSERVER);
				DisconnectUser(dwConnectionIdx);
				return;
			}

			MSG_DWORD3BYTE2 msg;
			msg.Category	= MP_USERCONN;
			msg.Protocol	= MP_USERCONN_NOTIFY_USERLOGIN_SYN;
			msg.dwObjectID	= UserIdx;
			msg.dwData1		= AuthKey;
			msg.dwData2		= dwConnectionIdx;
			msg.dwData3		= (DWORD)nTime;
			msg.bData1		= (BYTE)UserLevel;
			msg.bData2		= (BYTE)wType;
				
			g_Network.Send2Server(pAgentServer->dwConnectionIndex,(char*)&msg,sizeof(msg));
				
			//g_pLoginUser.Remove( pUserInfo->dwUserID );
		}
		break;
	case 2: // ID¾ø?½.
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);
		}
		break;
	case 3: // ºñ¹Ð¹øÈ£¿?·ù.
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);			
		}
		break;
	case 4: // °Ô?Ó¿¡ Á¢¼ÓÁß.
		{
			if(g_nServerSetNum == ServerNo)
			{
				MSG_DWORD msg;
				msg.Category = MP_USERCONN;
				msg.Protocol = MP_USERCONN_NOTIFY_OVERLAPPEDLOGIN;
				msg.dwData = UserIdx;
				g_Network.Broadcast2AgentServer((char*)&msg,sizeof(msg));
			
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGIN,0);
			}
			else
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGININOTHERSERVER,ServerNo);
			}
		}
		break;

	case 5: // ºí·°µÈ °èÁ¤. @state_num = 6?Ì¸é °èÁ¤ºí·° (?¥ °ü¸®¸ðµå¿¡¼­ »ç¿ë)
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_BLOCKUSERLEVEL);
		}
		break;

	case 7: // DB Connection Error
		{
			ASSERTMSG(0, "login check error ret 0");
		}
		break;

	case 8: // ?Î¿øÃÊ°ú
		{			
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MAXUSER );
		}
		break;
	case 14:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOT_CLOSEBETAWINNER );
		}
		break;
	case 15: //¼º?Î¼·¿¡ ¹Ì¼º³â?Ú°¡ µé¾î¿?·Á°í Çß´Ù.
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MINOR_INADULTSERVER );
		}
		break;
	case 50: // È¸¿øÅ»ÅðÇÑ °èÁ¤?Ó.
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_SECEDEDACCOUNT);
		}
		break;
	case 999:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOIPREGEN );
		}
		break;
	default:
		{
			ASSERT(0);
		}
	}
}

// for Japan
void LoginCheckQueryForJP( char* id, char* pw, char* ip, WORD ServerNo, DWORD AuthKey, DWORD dwConnectionIndex, DWORD dwMaxUser, WORD ServerNum )
{
	char guery[128];
	// id,pw,AuthKey,ServerGroupNum
	// MaxUser°Ë»çÇÒ¶§
	sprintf(guery, "EXEC up_gameloginadult_free_JP \'%s\', \'%s\', \'%s\', %d, %d, %d, %d, %d", 
				   id, pw, ip, ServerNo, AuthKey, dwMaxUser, gUserMGR.GetLimitAge(), ServerNum);
	if(g_DB.LoginQuery(eQueryType_FreeQuery, eLoginCheckQueryForJP, dwConnectionIndex, guery, FALSE) == FALSE)
	{
		g_Console.Log(eLogDisplay,4,"DB is Busy  ID:%d  ConnectionIndex:%d",id,dwConnectionIndex);
		LoginError( dwConnectionIndex, AuthKey, LOGIN_ERROR_DISTSERVERISBUSY );
		return;
	}
}

void RLoginCheckQueryForJP( LPQUERY pData, LPDBMESSAGE pMessage )
{
	DWORD dwConnectionIdx	= pMessage->dwID;
	DWORD stateNo			= atoi((char*)pData->Data[0]);
	WORD ServerNo			= atoi((char*)pData->Data[1]);
	DWORD UserIdx			= atoi((char*)pData->Data[2]);
	WORD wType				= atoi((char*)pData->Data[3]);
	int nTime				= atoi((char*)pData->Data[4]);
//	WORD bTen				= atoi((char*)pData->Data[5]);
	DWORD AuthKey			= atoi((char*)pData->Data[6]);
	BYTE UserLevel			= atoi((char*)pData->Data[7]);
	WORD AgentNo			= atoi((char*)pData->Data[8]);

	USERINFO* pUserInfo = g_pUserTable->FindUser(dwConnectionIdx);
	if(pUserInfo == NULL)
	{
		//���� �������� ���� �α��� delete�� ���ش�.
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	if(pUserInfo->dwUniqueConnectIdx != AuthKey)		// ?Ì¹Ì ³ª°¡°í ´Ù¸¥ ³Ñ?Ì µé¾î¿Â °æ¿ì
	{
		//���� �������� ���� �α��� delete�� ���ش�.		
		if( stateNo == 1 ) LoginCheckDelete(UserIdx);
		return;
	}

	pUserInfo->dwUserID = UserIdx;
	pUserInfo->UserLevel = UserLevel;
	pUserInfo->State = stateNo;
	
	switch(stateNo)
	{
	case 1: // success
		{
			pUserInfo->dwDisconOption |= eDISCONOPT_DELETELOGINCHECK;	//AAA
			//�������� ����� DeleteLoginCheck�� ���ְ� �ȴ�. 
			//�����κп��� ȣ���ϴ� ���� OndisconnectUser���� ȣ���ϵ��� �����Ѵ�.

			g_LoginUser.Remove( UserIdx );

			// check userlevel
			if(gUserMGR.IsValidUserLevel(UserLevel) == FALSE)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_INVALIDUSERLEVEL);

				DisconnectUser(dwConnectionIdx);
				return;
			}

			if( UserLevel == eUSERLEVEL_GM )
			{
				char strIP[16] = {0,};
				WORD wPort;
				g_Network.GetUserAddress( dwConnectionIdx, strIP, &wPort );

				IPCheckQuery( strIP, AuthKey, dwConnectionIdx );
				return;
			}

			SERVERINFO* pAgentServer = g_pServerTable->GetServer(AGENT_SERVER, AgentNo);
			if(pAgentServer == NULL)
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_NOAGENTSERVER);
				DisconnectUser(dwConnectionIdx);
				return;
			}

			MSG_DWORD3BYTE2 msg;
			msg.Category	= MP_USERCONN;
			msg.Protocol	= MP_USERCONN_NOTIFY_USERLOGIN_SYN;
			msg.dwObjectID	= UserIdx;
			msg.dwData1		= AuthKey;
			msg.dwData2		= dwConnectionIdx;
			msg.dwData3		= (DWORD)nTime;
			msg.bData1		= UserLevel;
			msg.bData2		= (BYTE)wType;
				
			g_Network.Send2Server(pAgentServer->dwConnectionIndex,(char*)&msg,sizeof(msg));
				
			//g_pLoginUser.Remove( pUserInfo->dwUserID );
		}
		break;
	case 2: // id error
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);
		}
		break;
	case 3: // password error
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_WRONGIDPW);			
		}
		break;
	case 4: // already in other server
		{
			if(g_nServerSetNum == ServerNo)
			{
				MSG_DWORD msg;
				msg.Category = MP_USERCONN;
				msg.Protocol = MP_USERCONN_NOTIFY_OVERLAPPEDLOGIN;
				msg.dwData = UserIdx;
				g_Network.Broadcast2AgentServer((char*)&msg,sizeof(msg));
			
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGIN,0);
			}
			else
			{
				LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_OVERLAPPEDLOGININOTHERSERVER,ServerNo);
			}
		}
		break;
	case 5: // block user
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_BLOCKUSERLEVEL);
		}
		break;
	case 7: // DB Connection Error
		{
			ASSERTMSG(0, "login check error ret 0");
		}
		break;
	case 8: // check maxuser
		{			
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_MAXUSER );
		}
		break;
	case 50: // no account
		{
			LoginError(dwConnectionIdx,AuthKey,LOGIN_ERROR_SECEDEDACCOUNT);
		}
		break;
	case 999:
		{
			LoginError( dwConnectionIdx, AuthKey, LOGIN_ERROR_NOIPREGEN );
		}
		break;
	default:
		{
			ASSERT(0);
		}
	}
}

#ifdef _NPROTECT_
//---for Hacktool User Check
void HackToolUser( DWORD UserIdx )
{
	char query[128];
	sprintf(query, "EXEC dbo.TP_HackUserInfoInsert %d, %d, \'%s\', %d", UserIdx, 0, "", g_nServerSetNum );
	g_DB.LoginQuery(eQueryType_FreeQuery, eHackToolUser, 0, query);
}
#endif
