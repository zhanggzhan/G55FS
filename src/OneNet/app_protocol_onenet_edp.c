///////////////////////////////////////////////////////////////////////////////
//
//   onenet 协议处理  
//
///////////////////////////////////////////////////////////////////////////////
#include "app_protocol_onenet_edp.h"
int16_t hex2str(INT8U *hex, INT16U len, INT8U *str);
extern volatile INT8S tcp_client_socket_id;
uint8_t gprs_send_app_data(void);
extern tagWirelessObj GprsObj;
uint8_t gprs_read_app_data(void);
int16_t gprs_send_cmd_wait_OK(uint8_t *cmd,uint8_t *resp,uint16_t max_resp_len,uint16_t timeout_10ms);
void gprs_power_off(void);
void remote_send_app_frame(uint8_t *frame,uint16_t frame_len);

INT8U edp_version[5] ={'0','7','1','0',0};
void responseCommandToOnenet(const char* cmdid, uint16_t cmdid_len,INT8U commandIndex)
{
	EdpPacket* pkg = NULL;
	INT8U posStart =0;
	INT8U suc[6] ={0x90,0x04,0x40,0x00,0x01,0x00};
	INT8U res=0;//判断平台是否返回结果
	INT8U tmp_has_dataToRead =0;
	//mem_set(EdpData,BUFFER_SIZE,0x00);

	INT32U resLen;
	
	//switch (commandIndex)
	//{
		//case  7:		//实时电能信息
		//while(!reportHoldDayEnergyToOnenet(0));
		//break;
		//case  11:		//实时日冻结信息
		//while(!reportHoldDayEnergyToOnenet(1));
		//break;
		//case 14:			 //获取电能信息数据块
		//while(!reportyEnergyBlockToOnenet());
		//break;
	//}
	//DelayNmSec(5000);
	while(!res)
	{
		mem_set(OneNetData,1024,0x00);
		if(commandIndex ==5)
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}
		if(commandIndex ==6)
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}

		if(commandIndex == 7) //召实时电能信息
		{
			resLen = cjsonEnergyInfoPack(0,OneNetData);;  //实时电能信息
		}
		if(commandIndex == 11) //召实时电能信息
		{
			resLen = cjsonEnergyInfoPack(1,OneNetData);  //实时日冻结信息
		}
		if(commandIndex == 12)
		{
			resLen = cjsonReadMeter(OneNetData);   //透传抄表数据。
		}
		if(commandIndex == 14)
		{
			resLen = cjsonEnergyBlockPack(OneNetData);  //获取电能信息数据块
		}
		if(commandIndex == 32)  //响应设置回应时间间隔的报文。
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}		
		if(commandIndex == 243)
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}
		if(commandIndex ==100)  //设置onenet平台服务器地址和端口
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}
		if(commandIndex ==101) // 设置升级跳转服务器和端口
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}
		if(commandIndex ==102)  //读取没有上报成功的数据命令
		{
			mem_set(OneNetData,4,0x00);
			resLen =4;
		}
		pkg = PacketCmdResp(cmdid, cmdid_len, OneNetData,resLen);
		
		
//		hex2str(pkg->_data,pkg->_write_pos,oneNetstr);
//		sprintf((char*)oneNetstr2,"AT+NSOSD=%d,%d,%s\r\n", tcp_client_socket_id,pkg->_write_pos,oneNetstr);
		remote_send_app_frame(pkg->_data,pkg->_write_pos);
//		sprintf((char*)oneNetstr2,"AT+NSOSD=%d,%d,%s\r\n", tcp_client_socket_id,pkg->_write_pos,oneNetstr);

//		gprs_send_cmd_wait_OK(oneNetstr2,gprs_cmd_buffer,sizeof(gprs_cmd_buffer),500);
		res =1;
	}
}

static int32_t find_first_num(INT8U *buf, INT16U len)
{
	INT16U i,find=0;
	INT32U num=0;
	for(i = 0; i<len; i++)
	{
		if(buf[i] >= '0' && buf[i]<= '9')
		{
			num = num*10;
			num += buf[i] - '0';
			find = 1;
		}
		else
		{
			if(find == 1)
			return num;
		}
	}
	return -1;
}
INT8U find_ip_port(INT8U *buf,INT16U len,INT8U *ip_port)
{
	INT16U i,find=0;
	INT16U num=0;
	INT8U ip_pos =0;
	INT8U transBegin =0;
	for(i = 0;i<len;i++)
	{
		if((buf[i] >= '0') && (buf[i]<= '9'))
		{
			transBegin =1;
			num = num*10;
			num += buf[i] - '0';
			continue;
		}
		if(transBegin)
		{
			if(buf[i]!='\"')
			{
				*(ip_port+ip_pos++) =(INT8U)num;
				num =0;
			}
			else 
			{
				*(ip_port+ip_pos++) =(INT8U)num;
				*(ip_port+ip_pos++) =(INT8U)(num>>8);
			}
		}
		
	}
	if(*ip_port !=0)
	{
		return 1;
	}
	return 0;
}
static uint8_t gprs_cmd_buffer2[200];
void registerDevice()
{    INT8U data[60]={0X10,0X2B,0X00,0X03,0X45,0X44,0X50,0X01,0XC0,0X01,0X2C,0X00,0X00,0X00,0X06,0X32,0X31,0X32,0X35,
					0X32,0X35,0X00,0X16};//0X31,0X31,0X31,0X31,0X31,0X31,0X31,0X31,0X31,0X31,0X31,0X31 0X31 0X31 0X31 0X31 0X31 0X31 0X31 0X31 0X31 0X31 };
	INT8U  str[200],str2[200];
	INT32U passMS=0;
	volatile INT8U tmp;
	INT8U* oneNetstr = NULL;
	INT8U* oneNetstr2 = NULL;
	INT8U idx=0,count=0;
	INT16U Gprs_recv_cnt =0 ;
//	mem_set(EdpData,BUFFER_SIZE,0x00);
    oneNetstr = ResponseApp.frame+500;
	oneNetstr2 =ResponseApp.frame+700;
	mem_set(oneNetstr,200,0x00);
	mem_set(oneNetstr2,200,0x00);
	for(idx=0;idx<22;idx++)
	{
		data[idx+23] = gSystemInfo.managementNum[idx];
	}

//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	gSystemInfo.edp_login_state = 0;
	gSystemInfo.dev_config_ack = 0;
	while(!gSystemInfo.edp_login_state)
	{
		GprsObj.send_len = 45;
		GprsObj.send_ptr = data;
		gprs_send_app_data();
		//remote_send_app_frame(data,45);	
		//gprs_send_app_data();
		passMS =system_get_tick10ms();
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.edp_login_state)) //小于10秒
		{
#ifdef DEBUG			
			system_debug_info("in loop login");
#endif		
			tmp =	gprs_read_app_data();
			system_debug_info("\xff\xff\xff");
			system_debug_data(&tmp,1);
			system_debug_info("\xff\xff\xff");
			//if(GprsObj.recv_pos -Gprs_recv_cnt)
			//{
				//mem_cpy(RequestRemote.frame,GprsObj.recv_buf + Gprs_recv_cnt,  GprsObj.recv_pos -Gprs_recv_cnt);
				//RequestRemote.frame_len =GprsObj.recv_pos -Gprs_recv_cnt;
				//app_protocol_handler_edp(&RequestRemote,NULL);
			//}
		}
		if(count++>3)
		{
			app_softReset();
		}
	}
//上电跟平台上的产品号建立连接后后先读onenet存着的一些数据，将其读空。然后执行后面的步骤。
{
	passMS =system_get_tick10ms();
	Gprs_recv_cnt = GprsObj.recv_pos ;
	while((second_elapsed(passMS)<15))
	{
		gprs_read_app_data();
		if(GprsObj.recv_pos != Gprs_recv_cnt)
		{
			passMS =system_get_tick10ms();
			Gprs_recv_cnt = GprsObj.recv_pos;
		}
		
#ifdef DEBUG
			system_debug_data(&Gprs_recv_cnt,2);
			system_debug_info("in 1111111");
			system_debug_data(&GprsObj.recv_pos,2);
#endif				
	}
}


}
/**
*定时发送心跳，暂时以2分钟为准
*/
INT8U edp_ping()
{
	INT8U count =0 ;
	INT32U passMS=0;
//	INT8U Gprs_recv_cnt =0 ;
//	INT8U tmp_has_dataToRead =0;  //因为has_data_to_read在gprs_read_app_data()中会清0，所以找个变量暂存下
	gSystemInfo.edp_ping_state =0;
	if(GprsObj.send_len)
	{
		return 0 ;
	}
	//
	mem_set(OneNetData,1024, 0x00);
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	while(!gSystemInfo.edp_ping_state)
	{
//		Gprs_recv_cnt = GprsObj.recv_pos ;
		EdpPacket* pkg =PacketPing();
		//GprsObj.send_len =pkg->_write_pos;
		//GprsObj.send_ptr = pkg->_data;
		//gprs_send_app_data();
		
		remote_send_app_frame(pkg->_data,pkg->_write_pos);
		system_debug_info("quit find edp ");
//		gprs_send_app_data();
		passMS =system_get_tick10ms();
#ifdef DEBUG
system_debug_info("in loop edp");
#endif		
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.edp_ping_state)) //小于10秒
		{
			
			//gprs_read_app_data();
			//if(GprsObj.recv_pos -Gprs_recv_cnt)
			//{
				//mem_cpy(RequestRemote.frame,GprsObj.recv_buf + Gprs_recv_cnt,  GprsObj.recv_pos -Gprs_recv_cnt);
				//RequestRemote.frame_len =GprsObj.recv_pos -Gprs_recv_cnt;
				//app_protocol_handler_edp(&RequestRemote,NULL);
			//}
		}
#ifdef DEBUG
system_debug_info("out loop edp");
#endif		
		if(count++>3)
		{
			gSystemInfo.tcp_link = 0;
			gSystemInfo.edp_ping_state =0;
			gSystemInfo.dev_config_ack = 0;
			break;
			//app_softReset();
		}
			
	}
	return 1;
}

/**
 * 上报设备配置信息到onenet平台，对平台下发的save_ack进行处理
 */
BOOLEAN reportConfigToOnenet()
{
  	INT8U count =0 ;
  	INT32U passMS=0;
	EdpPacket* pkg;
  	gSystemInfo.save_ack =0;
  	if(GprsObj.send_len)
  	{
	  	return  0 ;
  	}
  	mem_set(OneNetData,1024, 0x00);
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	gSystemInfo.dev_config_ack = 0;
  	while(!gSystemInfo.save_ack)
  	{
		cjsonDeviceConfigPack(OneNetData);
	  	 pkg = PacketSavedataJson2(NULL, OneNetData, 1, 1);
	  	GprsObj.send_len =pkg->_write_pos;
	  	GprsObj.send_ptr = pkg->_data;
	  	gprs_send_app_data();
	  	passMS =system_get_tick10ms();
	  	while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
	  	{
#ifdef DEBUG			 
		  	//system_debug_info("in loop configration");
#endif			
			gprs_read_app_data();  
	  	}
	  	if(count++>3)
	  	{
			gSystemInfo.tcp_link = 0;
			gSystemInfo.edp_login_state =0;
			gSystemInfo.dev_config_ack = 0;
			break;
			//gprs_power_off();	
			//app_softReset();
	  	}
	  	 gSystemInfo.dev_config_ack = 1;
		 edp_ping_start_time =system_get_tick10ms();//

  	}
	 
	return 1;
}


/**
 * 上报设备正向有功信息到onenet平台，对平台下发的save_ack进行处理
 */
INT8U reportHoldDayEnergyToOnenet(INT8U flag)
{
	INT8U count =0 ;
	DateTime dt;
	tpos_datetime(&dt);
	INT64U report_time =0; //延时之后需要上报的时刻，除了这个还有加上一个延时时间。
	INT32U passMS=0;
	EdpPacket* pkg;
	gSystemInfo.save_ack =0;
	//if(!gSystemInfo.tcp_link) //如果没有登录成功，那么直接存储
	//{
//#ifdef DEBUG			 
		  	//system_debug_info("\r\n***************store hold day energy****************\r\n");
//#endif			
		//cjsonEnergyInfoPack(flag,OneNetData);
		//pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		//handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。
		//return 1;
	//}
	if(wait_report_flag)  //需要直接存储起来。以备延时使用。
	{
#ifdef DEBUG
		system_debug_info("\r\n***************waite report modestore hold day energy****************\r\n");
#endif
		cjsonEnergyInfoPack(flag,OneNetData);
		pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		mem_cpy_right(pkg->_data+8,pkg->_data,pkg->_write_pos);
		if(flag == 1)
		{
			report_time = getPassedSeconds(&dt,2000)+(getrand()%60)*60;	
		}
		else if(flag == 0)
		{
			report_time = getPassedSeconds(&dt,2000)+getrand()%(gSystemInfo.edp_rand_report_end -gSystemInfo.edp_rand_report_begin)*60 + getrand()%60;
		}

		mem_cpy(pkg->_data,(INT8U*)&report_time,8);
		handle_wait_report_data_write(pkg->_data,pkg->_write_pos+8);//如果没上传成功那么存储下来。
		return 1;
	}
	if(GprsObj.send_len)
	{
		return  0 ;
	}
	if(!cmdType)   //如果是对命令召测的回应，那么就不要再去重新读数据了，因为前面的命令数据已经先回复了，那么只需要拿过来转发就可以了
	{
		mem_set(OneNetData,1024, 0x00);  	
	}
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	while(!gSystemInfo.save_ack)
	{
		if(!cmdType)//如果是对命令召测的回应，那么就不要再去重新读数据了，因为前面的命令数据已经先回复了，那么只需要拿过来转发就可以了
		{
			cjsonEnergyInfoPack(flag,OneNetData);
		}

		EdpPacket* pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		GprsObj.send_len =pkg->_write_pos;
		GprsObj.send_ptr = pkg->_data;
		gprs_send_app_data();
		passMS =system_get_tick10ms();
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
		{

			gprs_read_app_data();
		}
#ifdef DEBUG
		system_debug_info("\r\n*********in report hold day energy **********");
#endif		
		if(count++>3)
		{
			handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。			
			gSystemInfo.tcp_link = 0;
			gSystemInfo.edp_login_state =0;
			gSystemInfo.dev_config_ack = 0;
			break;
			//gprs_power_off();
			//app_softReset();
			
		}
		  	
	}
	return 1;
}

/**
 * 上报主站的透传数据到平台，对平台下发的save_ack进行处理
 */
INT8U report645CmdResultToOnenet()
{
	INT8U count =0 ;
	INT32U passMS=0;
	EdpPacket* pkg;
	gSystemInfo.save_ack =0;
	if(!gSystemInfo.tcp_link) //如果没有登录成功，那么直接存储
	{
#ifdef DEBUG			 
		  	system_debug_info("\r\n***************store hold day energy****************\r\n");
#endif			
		cjsonReadMeter(OneNetData);
		pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。
		return 1;
	}
	if(GprsObj.send_len)
	{
		return  0 ;
	}
	if(!cmdType)   //如果是对命令召测的回应，那么就不要再去重新读数据了，因为前面的命令数据已经先回复了，那么只需要拿过来转发就可以了
	{
		mem_set(OneNetData,1024, 0x00);  	
	}
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	while(!gSystemInfo.save_ack)
	{
		if(!cmdType)//如果是对命令召测的回应，那么就不要再去重新读数据了，因为前面的命令数据已经先回复了，那么只需要拿过来转发就可以了
		{
			cjsonReadMeter(OneNetData);
		}

		EdpPacket* pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		GprsObj.send_len =pkg->_write_pos;
		GprsObj.send_ptr = pkg->_data;
		gprs_send_app_data();
		passMS =system_get_tick10ms();
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
		{

			gprs_read_app_data();
		}
#ifdef DEBUG
		system_debug_info("\r\n*********in report hold day energy **********");
#endif		
		if(count++>3)
		{
			handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。			
			gSystemInfo.tcp_link = 0;
			gSystemInfo.edp_login_state =0;
			gSystemInfo.dev_config_ack = 0;
			break;
			//gprs_power_off();
			//app_softReset();
			
		}
		  	
	}
	return 1;
}
/**
 * 上报设备正向有功信息到onenet平台，对平台下发的save_ack进行处理
 */
INT8U reportyEnergyBlockToOnenet()
{
	INT8U count =0 ;
	DateTime dt;
	tpos_datetime(&dt);
	INT64U report_time =0; //延时之后需要上报的时刻，除了这个还有加上一个延时时间。	
	INT32U passMS=0;
	EdpPacket* pkg;
	gSystemInfo.save_ack =0;
	
	//if(!gSystemInfo.tcp_link) //如果没有登录成功，那么直接存储
	//{
//#ifdef DEBUG			 
		  	//system_debug_info("\r\n***************store energy block****************\r\n");
//#endif		
		//cjsonEnergyBlockPack(OneNetData);
		//pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		//handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。
		//return 1;
	//}	
	if(wait_report_flag)  //需要直接存储起来。以备延时使用。
	{
#ifdef DEBUG
		system_debug_info("\r\n***************waite report mode store energy block****************\r\n");
#endif
		cjsonEnergyBlockPack(OneNetData);
		pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		mem_cpy_right(pkg->_data+8,pkg->_data,pkg->_write_pos);
		report_time = getPassedSeconds(&dt,2000)+getrand()%(gSystemInfo.edp_rand_report_end -gSystemInfo.edp_rand_report_begin)*60 + getrand()%60;
		mem_cpy(pkg->_data,(INT8U*)&report_time,8);
		handle_wait_report_data_write(pkg->_data,pkg->_write_pos+8);//把数据存储在待上传列表中，时间到了再上传。
		return 1;
	}	

	if(GprsObj.send_len)
	{
		return  0 ;
	}
	if(!cmdType)   //如果是对命令召测的回应，那么就不要再去重新读数据了，因为前面的命令数据已经先回复了，那么只需要拿过来转发就可以了
	{
		mem_set(OneNetData,1024, 0x00);
	}	  	
#ifdef DEBUG
		system_debug_info("\r\n***************in  energy block report1 ****************\r\n");
#endif	
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	while(!gSystemInfo.save_ack)
	{
		if(!cmdType)   //如果是对命令召测的回应，那么就不要再去重新读数据了，因为前面的命令数据已经先回复了，那么只需要拿过来转发就可以了
		{		
		cjsonEnergyBlockPack(OneNetData);
		}
		pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		GprsObj.send_len =pkg->_write_pos;
		GprsObj.send_ptr = pkg->_data;
		gprs_send_app_data();
		passMS =system_get_tick10ms();
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
		{
			#ifdef DEBUG
			system_debug_info("\r\n*********in  energy block report2 **********");
			#endif
			gprs_read_app_data();
		}
		if(count++>3)
		{
			handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。
			gSystemInfo.tcp_link = 0;
			gSystemInfo.edp_login_state=0;
			gSystemInfo.dev_config_ack = 0;
			break;
			//gprs_power_off();
			//app_softReset();

		}
		  	
	}
	return 1;
}
/**
 * 上报设备升级信息到onenet平台，对平台下发的save_ack进行处理
 */
INT8U responseUpdataResToOnenet(INT8U flag)
{
	INT8U count =0 ;
	INT32U passMS=0;
	gSystemInfo.save_ack =0;
	if(GprsObj.send_len)
	{
		return  0 ;
	}
	mem_set(OneNetData,1024, 0x00);  	  	
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	while(!gSystemInfo.save_ack)
	{
		cjsonFirmwareUpdatePack(OneNetData,flag);
		EdpPacket* pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		GprsObj.send_len =pkg->_write_pos;
		GprsObj.send_ptr = pkg->_data;
		gprs_send_app_data();
		passMS =system_get_tick10ms();
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
		{

			gprs_read_app_data();
		}
		#ifdef DEBUG
		system_debug_info("\r\n*********in report updata info **********");
		#endif
		if(count++>3)
		{
			gSystemInfo.tcp_link = 0;
			break;
			//gprs_power_off();
			//app_softReset();
		}
	}
	return 1;
}
INT32U  str2hexGen(INT8S *str)
{
	INT32U value,idx;
	BOOLEAN  start;

	value = 0;

	//数值前面的空格可以掠过
	start = TRUE;
	for(idx=0;idx<10;idx++)
	{
		if(*str==' ')
		{
			if(!start) break;
		}
		else if((*str < '0')  || (*str > '9') )
		{
			break;
		}
		else
		{
			start = FALSE;
			value *= 16;
			value += *str-'0';
		}
		str++;
	}
	return value;
}
uint8_t update_ertu_datetime(uint8_t force_flag);
void app_protocol_handler_edp(objRequest* pRequest,objResponse *pResp)
{
	INT8U tmp_updata_verison[4] ={0};
	EdpPacket  pkg;
	INT8U cmdid[100],req[500]={0},idx,idx1=0,cs=0,flag =0;
	INT8U str[50]={0},hex[50]={0},tmp[50]={0},pos1=0,pos2=0,tmpVersion;
	uint16 cmdid_len,req_len,serverDataPos,serverCommand=0,serverData=0;
	INT16S serverCommandPos =0;
	INT8U tmpPos=0,tmpPos2 = 0,rand_loop =0;
	INT8U rand_report_begin =0,rand_report_end =0;
	volatile INT8U data_tmp =0;
	INT8U ip_port[6] ={0};
	if((!gSystemInfo.dev_config_ack) &&(CMDREQ ==pRequest->frame[0]))  //还没有登录服务器不执行命令
	{
#ifdef DEBUG
		system_debug_info("\r\n***********dont to handle command no register*********\r\n");
#endif		
		return;
	}
#ifdef DEBUG
		system_debug_info("\r\n***********to handle command no register*********\r\n");
#endif
	pkg._data = pRequest->frame;
	pkg._write_pos = pRequest->recv_len;
	pkg._read_pos = 0;
	pkg._capacity = BUFFER_SIZE;
	switch(pkg._data[pkg._read_pos])
	{
		case CONNREQ: /* 连接请求 */
		break;
		case CONNRESP: /* 连接响应 */
			if(compare_string(pkg._data,"\x20\x02\x00\x00",4) ==0)
			{
				gSystemInfo.login_status = 1;
				gSystemInfo.edp_login_state =1;
			}
#ifdef DEBUG
			system_debug_info("\r\n***********already connect to onenet*********\r\n");
#endif		
			break;
		case PUSHDATA: /* 转发(透传)数据 */
		break;
		case SAVEDATA:/* 存储(转发)数据 */
		break;
		case SAVEACK: /* 存储确认 */
			if(compare_string(pkg._data,"\x90\x04\x40\x00\x01\x00",6) ==0)
			{
				gSystemInfo.save_ack =1;
			}
#ifdef DEBUG			
			system_debug_info("\r\n***********already store ack*********\r\n");
#endif			
			break;
		case  CMDREQ:/* 命令请求 */
			UnpackCmdReq(&pkg, cmdid, &cmdid_len,req, &req_len);
		if((serverCommandPos = str_find(req, req_len, "\"ServerCommand\":{\"int\":", 23)) >= 0)
			{
				serverCommand = find_first_num(req+serverCommandPos, req_len);
			}
			system_debug_info("\r\nMglget command server command");
			switch(serverCommand)
			{
				case 5: //修改设备采集数据的时间间隔
					responseCommandToOnenet(cmdid, cmdid_len,5);
					if((serverCommandPos = str_find(req, req_len, "\"ServerData\":{\"string\":", 20)) >= 0)
					{
						serverData = find_first_num(req+serverCommandPos, req_len);
						fwrite_ertu_params(EEADDR_TIME_INTERVAL,&serverData,2);
						gSystemInfo.edp_report_interval =(INT8U)serverData;
					}
					break;
					case 6: //同步设备时间
						responseCommandToOnenet(cmdid, cmdid_len,6);
						if((pos1 = str_find(req, req_len, "\"string\":\"", 10)) >= 0) //17-06-14 14:45:05
						{
							pos1+=10;
							for(idx=0;idx<17;)
							{
								if((req[pos1+idx]>='0')&&(req[pos1+idx]<='9'))
								{
									hex[idx1++] = (req[pos1+idx]-'0')*16+(req[pos1+idx+1]-'0');
									idx+=2;
								}
								else
								{
									idx++;
								}
							}
							idx = 0; //组报文初始位置
							str[idx++] =0x68;
							str[idx++] =gSystemInfo.meter_no[0];str[idx++] =gSystemInfo.meter_no[1];str[idx++] =gSystemInfo.meter_no[2];
							str[idx++] =gSystemInfo.meter_no[3];str[idx++] =gSystemInfo.meter_no[4];str[idx++] =gSystemInfo.meter_no[5];
							str[idx++] = 0x68;
							str[idx++] = 0x14;
							str[idx++] = 0x10;
							str[idx++] = 0x34;
							str[idx++] = 0x34;
							str[idx++] = 0x33;
							str[idx++] = 0x37;
							str[idx++] =0x35;str[idx++] =0x89;str[idx++] =0x67;str[idx++] =0x45;
							str[idx++] =0xAB;str[idx++] =0x89;str[idx++] =0x67;str[idx++] =0x45;
							hex[6]=weekDay(BCD2byte(hex[0]),BCD2byte(hex[1]),BCD2byte(hex[2]));//week
							str[idx++] =hex[6] +0x33;str[idx++] = hex[2] +0x33;
							str[idx++] =hex[1] +0x33;str[idx++] = hex[0] +0x33;
							cs = 0;
							for(pos1 =0;pos1<idx;pos1++) cs += str[pos1];
							str[idx++] = cs;
							str[idx++] =0x16;
							app_trans_send_meter_frame(str,28, tmp,200,100);
				
							idx = 0; //组报文初始位置
							str[idx++] =0x68;
							str[idx++] =gSystemInfo.meter_no[0];str[idx++] =gSystemInfo.meter_no[1];str[idx++] =gSystemInfo.meter_no[2];
							str[idx++] =gSystemInfo.meter_no[3];str[idx++] =gSystemInfo.meter_no[4];str[idx++] =gSystemInfo.meter_no[5];
							str[idx++] = 0x68;
							str[idx++] = 0x14;
							str[idx++] = 0x0F;
							str[idx++] = 0x35;
							str[idx++] = 0x34;
							str[idx++] = 0x33;
							str[idx++] = 0x37;
							str[idx++] =0x35;str[idx++] =0x89;str[idx++] =0x67;str[idx++] =0x45;
							str[idx++] =0xAB;str[idx++] =0x89;str[idx++] =0x67;str[idx++] =0x45;
							str[idx++] = hex[5] +0x33;
							str[idx++] = hex[4] +0x33;
							str[idx++] = hex[3] +0x33;
							cs = 0;
							for(pos1 =0;pos1<idx;pos1++) cs += str[pos1];
							str[idx++] = cs;
							str[idx++] =0x16;
							app_trans_send_meter_frame(str,27, tmp,200,100);
						}
						update_ertu_datetime(1);
						break;
					case 7: //采集当前实时电量
						responseCommandToOnenet(cmdid, cmdid_len,7);
						cmdType =7;
						break;
					case 11://采集日冻结电量
						responseCommandToOnenet(cmdid, cmdid_len,11);
						cmdType=11;
						break;
					case 12: //透传数据
						cmd645 = 0;
						if((serverCommandPos = str_find(req, req_len, "\"ServerData\":{\"string\":", 22)) >= 0)
						{
							tmpPos =24+serverCommandPos;
							cmd645 = str2hexGen(req+tmpPos);
						}
						responseCommandToOnenet(cmdid, cmdid_len,12);
						cmdType =12;
						break;
					case 14: //获取电能信息数据块
						system_debug_info("\r\nMglget command14:");
						responseCommandToOnenet(cmdid, cmdid_len,14);
						cmdType =14;
						break;
					case 15: //平台收到告警事件后响应
				
						break;
					case 32:  //设置随机延后时间范围
						if((serverCommandPos = str_find(req, req_len, "\"ServerData\":{\"string\":", 22)) >= 0)
						{
							tmpPos =24;
							tmpPos2 =24 ;
							for(rand_loop =0;rand_loop<3;rand_loop++)
							{
								data_tmp = req[tmpPos+serverCommandPos];
								if((data_tmp>='0')&&(data_tmp<='9'))
								{
									rand_report_begin = rand_report_begin*10+ data_tmp -0x30;
									tmpPos++;
								}
								else
								{
									tmpPos++;
									break;
								}
							}
							for(rand_loop =0;rand_loop<3;rand_loop++)
							{
								data_tmp = req[tmpPos+serverCommandPos];
								if((data_tmp>='0')&&(data_tmp<='9'))
								{
									rand_report_end = rand_report_end*10+ data_tmp -0x30;
									tmpPos++;
								}
								else
								{
									tmpPos++;
									break;
								}
							}
							fwrite_ertu_params(EEADDR_EDP_RAND_BEGIN,&rand_report_begin,1);
							fwrite_ertu_params(EEADDR_EDP_RAND_END,&rand_report_end,1);	
							gSystemInfo.edp_rand_report_begin =(INT8U)rand_report_begin;	
							gSystemInfo.edp_rand_report_begin =(INT8U)rand_report_end;						
						}
						cmdType = 32;
						break;
					case 242 : //重启设备
						app_softReset();
						break;
					case 243:   //固件升级命令
						responseCommandToOnenet(cmdid, cmdid_len,243);
						if((serverCommandPos = str_find(req, req_len, "BETA", 4)) >= 0)
						{
							fwrite_ertu_params(EEADDR_UPDATA_VERSION,req+serverCommandPos+4,4);
							cmdType = 243;
						}
						break;
					case 100:   // 设置onenet平台服务器地址和端口；
						responseCommandToOnenet(cmdid, cmdid_len,100);
						if((serverCommandPos = str_find(req, req_len, "\"ServerData\":{\"string\":", 23)) >= 0)
						{
							find_ip_port(req+serverCommandPos, req_len,ip_port);
						    fwrite_ertu_params(EEADDR_IP_PORT_ONENET, ip_port,6);
						}
						break;
					case 101:   //  设置升级跳转服务器和端口
						responseCommandToOnenet(cmdid, cmdid_len,101);
						if((serverCommandPos = str_find(req, req_len, "\"ServerData\":{\"string\":", 23)) >= 0)
						{
//							serverData = find_first_num(req+serverCommandPos, req_len);
							find_ip_port(req+serverCommandPos, req_len,ip_port);
						    fwrite_ertu_params(EEADDR_IP_PORT_UPDATE, ip_port,6);
						}
						break;
					case 102:   //  读取没有上报成功的数据命令
						break;
					default:
					break;
					}
					break;
					case CMDRESP:/* 命令响应 */
					break;
					case PINGREQ:/* 心跳请求 */
					break;
					case PINGRESP:/* 心跳响应 */
						if(compare_string(pkg._data,"\xD0\x00",2) ==0)
						{
							gSystemInfo.edp_ping_state =1;
						}
#ifdef DEBUG
						system_debug_info("********already finish heart response*******");
#endif						
						break;
					break;
					default :
					break;
			}

}
//*******************************开始处理未成功上报数据包的存储和上电后重发******************************************//
uint16_t nor_flash_read_data(uint16_t sector,uint16_t offset,uint8_t *data,uint16_t len);
uint16_t nor_flash_erase_page(uint16_t sector);
uint16_t nor_flash_write_data(uint16_t sector,uint16_t offset,uint8_t *data,uint16_t len);
INT8U edpStoreDataState[80]={0xFF};       //未上报成功数据存储
INT8U edpStoreWaitDataState[40]={0xFF};   //延时上报数据存储
#define  STORE_RECORD_START   4000
void first_poweron_flash_rebuilt()
{
	INT8U buftmp[8]={0},state_idx =0,page=0;
	static INT8U first_poweron =0;
	if(!first_poweron)  //首次上电重构
	{
		for(page = 0;page<FLASH_EDP_STORT_DATA_SECTOR_CNT;page++)
		{
			nor_flash_read_data(FLASH_EDP_STORT_DATA_START+page,STORE_RECORD_START,buftmp,8);
			
			for(state_idx =0;state_idx<8;state_idx++)
			{
				if(buftmp[state_idx] == 0x0)
				{
					nor_flash_erase_page(FLASH_EDP_STORT_DATA_START+page);
					mem_set(edpStoreDataState[8*page],8,0x00);
					break;
				}
				
				if(buftmp[state_idx]==0x1)
				{
					edpStoreDataState[8*page+state_idx] =0x1;
				}
				else
				{
					edpStoreDataState[8*page+state_idx] =0x0;
				}
			}
		}
	}
	first_poweron = 1;
}
//首次上电对延时上报flash中的数据进行重构，重构后的结果放到rand_wait_report_time_index中。
void first_poweron_flash_wait_report_rebuilt()
{
	INT8U buftmp[8]={0},state_idx =0,idx=0,page =0;
	static INT8U first_poweron =0;
	if(!first_poweron)  //首次上电重构
	{
		for(idx = 0;idx<FLASH_EDP_RAND_REPORT_STORE_CNT*8;idx++)
		{
			nor_flash_read_data(FLASH_EDP_RAND_REPORT_STORE_START+idx/8,500*(idx%8),buftmp,8);
			
			if(mem_all_is(buftmp,8,0x00)||mem_all_is(buftmp,8,0xFF))
			{
				rand_wait_report_time_index[idx] = 0;
			}
			else
			{
				rand_wait_report_time_index[idx] =*((INT64U*)&buftmp);
			}
		}
		for(page = 0;page<FLASH_EDP_RAND_REPORT_STORE_CNT;page++)
		{
			nor_flash_read_data(FLASH_EDP_RAND_REPORT_STORE_START+page,STORE_RECORD_START,buftmp,8);
			
			for(state_idx =0;state_idx<8;state_idx++)
			{
				//if(buftmp[state_idx] == 0x0)  不存在这种情况
				//{
					//nor_flash_erase_page(FLASH_EDP_RAND_REPORT_STORE_CNT+page);
					//mem_set(edpStoreWaitDataState[8*page],8,0x00);
					//break;
				//}
				if(buftmp[state_idx]==0x1)
				{
					edpStoreWaitDataState[8*page+state_idx] =0x1;
				}
				else
				{
					edpStoreWaitDataState[8*page+state_idx] =0x0;
				}
			}
		}
	}
	first_poweron = 1;
}
/**
*@brief  handle some message that doesn't report to onenet. read some message from flash.
*/
INT8U report_buf[500] ={0xFF};
void handleUnsuccessReportDataRead()
{
	INT8U pos =0,page=0,i=0;
	INT8U buftmp[8]={0x0},find_pos_state=0,find_pos=0,has_find =0,store_flag =0;
	INT32U passMS =0;
	INT8U count =0;
	INT16U reportDataLen = 500,idx =0;
	if(mem_all_is(edpStoreDataState,80,0x00))
	{
		return;
	}	
	else
	{
		//********************找到最老的那个需要上传的数据***************************//
		for(i=0;i<80;)
		{
			has_find = 0;
			switch (find_pos_state)
			{
				case 0:
					if(edpStoreDataState[i++] ==0x01)
					find_pos_state =1;
					break;
				case 1:
					if(edpStoreDataState[i++] ==0x00)
					find_pos_state =2;
					if(i==80)
					{
						i=0;
					}
					break;
				case 2:
					if(edpStoreDataState[i] == 0x01)
					{
						find_pos = i;
						has_find = 1;
					}
					i++;
					if(i==80)
					{
						i=0;
					}
					break;
				default:
					break;
			}
			if(has_find)  //找到了！！！
			{
				break;
			}
		} // 结束for循环	
		nor_flash_read_data(FLASH_EDP_STORT_DATA_START+find_pos/8,500*(find_pos%8),report_buf,500);
		if(!IsPkgComplete(report_buf))  //判断存储在未上报成功的flash中的数据，如果不是edp格式的，那么不上报
		{
			store_flag =0x00;   //store_flag=0 代表该点的数据已经上报了
			nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
			return;
		}
		for(idx=500-1;idx>0;idx--)  //得到要回复的报文的长度。
		{
			if(*(report_buf+idx) == 0xff)
			{
				reportDataLen --;
			}
			else
			{
				break;
			}
		}
		if(GprsObj.send_len)
		{
			return  ;
		}
		if(report_buf[0] ==0xC0)   //如果flash中存入了edp ping，这是个错误，应该寻找下从哪存的，就不上报了，直接返回，flash清除存储。
		{
			store_flag =0x00;   //store_flag=0 代表该点的数据已经上报了
			nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
			edpStoreDataState[find_pos] = 0;
			return  ;
		}
		gSystemInfo.save_ack =0;
#ifdef DEBUG
		system_debug_info("\r\n===========handle report  unsuccessful data in flash=========\r\n");
#endif		
		while(!gSystemInfo.save_ack)
		{
			GprsObj.send_len =reportDataLen;
			GprsObj.send_ptr = report_buf;
			gprs_send_app_data();
#ifdef DEBUG
			system_debug_info("\r\n*******send ok ******\r\n");
#endif			
			passMS =system_get_tick10ms();
			while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
			{

				gprs_read_app_data();
			}
				#ifdef DEBUG
				system_debug_info("\r\n*********in report report store info **********");
				#endif			
			if(count++>3)
			{
				gSystemInfo.tcp_link = 0;
				gSystemInfo.edp_login_state=0;
				gSystemInfo.dev_config_ack = 0;
				break;
				//app_softReset();
			}
		}
		if(gSystemInfo.save_ack)
		{
			store_flag =0x00;   //store_flag=0 代表该点的数据已经上报了
			nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
			edpStoreDataState[find_pos] = 0;
		}
	}
	return ;	
}
/**
*@brief  handle some message that doesn't report to onenet. write some message to flash.
*/
//INT8U buftmp[4096] ={0};
void handleUnsuccessReportDataWrite(INT8U *buf,INT16U len)
{
	INT8U i=0,has_find =0,find_pos_state =0,find_pos =0,store_flag=0;  //这里store_flag既用于存储标志，又用于擦除块的计算。
	//********************找到最老的那个需要上传的数据***************************//
	store_flag = 1;
	for(i=0;i<80;)
	{
		has_find = 0;
		switch (find_pos_state)
		{
			case 0:
				if(edpStoreDataState[i++] ==0x01)
				find_pos_state =2;
				break;
			case 2:
				if(edpStoreDataState[i] == 0x00)
				{
					find_pos = i;
					has_find = 1;
				}
				i++;
				if(i==80)
				{
					i=0;
				}
				break;
			default:
				break;
		}
		if(has_find)  //找到了！！！
		{
			break;
		}
	} // 结束for循环	
	if(find_pos ==0)  //每次要往第一个块里写的时候，需要擦一下第一个块的数据。
	{
		nor_flash_erase_page(FLASH_EDP_STORT_DATA_START);
#ifdef DEBUG
	system_debug_info("\r\n**************************need to ereaser a new block 0 ****************************\r\n");
#endif		
	}
	if(find_pos%8 == 7) //说明本次存储的位置是每一个sector的最后一块。//存完之后还有将下一个块擦掉
	{
		nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,500*(find_pos%8),buf,len);
		nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
		edpStoreDataState[find_pos] = 1;
		if(find_pos==79)
		{
			store_flag = 0;
		}
		else
		{
			store_flag = find_pos/8+1;	
		}
		nor_flash_erase_page(FLASH_EDP_STORT_DATA_START+store_flag);
		mem_set(edpStoreDataState+8*store_flag,8,0x00);
#ifdef	DEBUG
		system_debug_info("\r\n**********************unsucessful store data need to change page*******************************\r\n");
#endif		
	}
	else //否则的话直接存储即可
	{
		nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,500*(find_pos%8),buf,len);
		nor_flash_write_data(FLASH_EDP_STORT_DATA_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
		edpStoreDataState[find_pos] = 1;
#ifdef	DEBUG
		system_debug_info("\r\n**********************unsucessful store data1*******************************\r\n");
#endif	
	}
}
/**
*@brief  rebuild the warning information from flash when first power on.
*        include overflow warning ,open meter warning ,clean meter waring.
*/
void powerOnRebuildWaningInfo()
{
	INT8U tmp[4]={0};
	INT8U str1[100],str2[100];
	INT8U len =0;
 	fread_ertu_params(EEADDR_OVERFLOW_WARNING_A,tmp,4);
	if(mem_all_is(tmp,4,0xFF))
	{
		mem_set(str1,100,0xFF);
		mem_set(str2,100,0xFF);
		len =app_read_his_item(0x19010001,str1,str2,255,0,2000);  //A相过流总次数 抄表的结果应该放在str1里。
		if(len !=0)
		{
			gSystemInfo.edp_over_flow_A =(BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			fwrite_ertu_params(EEADDR_OVERFLOW_WARNING_A,(INT8U*)(&gSystemInfo.edp_over_flow_A),4);	
		}

	}
	else
	{
		gSystemInfo.edp_over_flow_A =bin2_int32u(tmp);
	}

	fread_ertu_params(EEADDR_OVERFLOW_WARNING_B,tmp,4);
	if(mem_all_is(tmp,4,0xFF))
	{
		mem_set(str1,100,0xFF);
		mem_set(str2,100,0xFF);
		len = app_read_his_item(0x19020001,str1,str2,255,0,2000);  //B相过流总次数 抄表的结果应该放在str1里。
		if(len !=0)
		{
			gSystemInfo.edp_over_flow_B =(BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			fwrite_ertu_params(EEADDR_OVERFLOW_WARNING_B,(INT8U*)(&gSystemInfo.edp_over_flow_B),4);
		}
	}
	else
	{
		gSystemInfo.edp_over_flow_B =bin2_int32u(tmp);
	}
	fread_ertu_params(EEADDR_OVERFLOW_WARNING_C,tmp,4);
	if(mem_all_is(tmp,4,0xFF))
	{
		mem_set(str1,100,0xFF);
		mem_set(str2,100,0xFF);
		len =app_read_his_item(0x19030001,str1,str2,255,0,2000);  //C相过流总次数 抄表的结果应该放在str1里。
		if(len!=0)
		{
			gSystemInfo.edp_over_flow_C =(BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			fwrite_ertu_params(EEADDR_OVERFLOW_WARNING_C,(INT8U*)(&gSystemInfo.edp_over_flow_C),4);
		}
	}
	else
	{
		gSystemInfo.edp_over_flow_C =bin2_int32u(tmp);
	}		
	fread_ertu_params(EEADDR_OPENMETER_WARNING,tmp,4);
	if(mem_all_is(tmp,4,0xFF))
	{
		mem_set(str1,100,0xFF);
		mem_set(str2,100,0xFF);
		len =app_read_his_item(0x03300D00,str1,str2,255,0,2000);  //开表盖 抄表的结果应该放在str1里。
		if(len !=0)
		{
			gSystemInfo.edp_open_meter =(BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			fwrite_ertu_params(EEADDR_OPENMETER_WARNING,(INT8U*)(&gSystemInfo.edp_open_meter),4);
		}
	}
	else
	{
		gSystemInfo.edp_over_flow_C =bin2_int32u(tmp);
	}	
	fread_ertu_params(EEADDR_CLEANNMETER_WARNING,tmp,4);
   if(mem_all_is(tmp,4,0xFF))
	{
		mem_set(str1,100,0xFF);
		mem_set(str2,100,0xFF);
		len =app_read_his_item(0x03300100,str1,str2,255,0,2000);  //清零 抄表的结果应该放在str1里。
		if(len !=0)
		{
			gSystemInfo.edp_clean_meter =(BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			fwrite_ertu_params(EEADDR_CLEANNMETER_WARNING,(INT8U*)(&gSystemInfo.edp_clean_meter),4);
		}
	}
	else
	{
		gSystemInfo.edp_clean_meter =bin2_int32u(tmp);
	}	
}
/**
*@brief  每隔30S处理edp协议的告警事件
*/
void warning_report_handler(INT8U *warningMask)
{
	INT8U loop =0;
	INT8U len =0;
	INT8U str1[100] ={0},str2[100]={0};
	INT32U cnt;
	if(*warningMask ==0)  //说明不需要上报告警。
	{
		return ;
	}
	for(loop=0;loop<5;loop++)
	{
		if(get_bit_value(warningMask,1,loop))
		{
			break;
		}
	}
	mem_set(str1,100,0xFF);
	mem_set(str2,100,0xFF);
	switch(loop)
	{
		case 0:
		    cnt =0;
			len =app_read_his_item(0x19010001,str1,str2,255,0,2000);  //A相过流总次数 抄表的结果应该放在str1里。
			if(!len)
			{
				cnt = (BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			}
			if(cnt != gSystemInfo.edp_over_flow_A) //如果读回来的告警次数不等于flash中存储的告警次数，那么需要上报平台了。
			{
				warning_report_cmd =1;
			}
			break;
		case 1:
			cnt =0;
			len =app_read_his_item(0x19020001,str1,str2,255,0,2000);  //B相过流总次数 抄表的结果应该放在str1里。
			if(!len)
			{
				cnt = (BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			}
			if(cnt != gSystemInfo.edp_over_flow_B) //如果读回来的告警次数不等于flash中存储的告警次数，那么需要上报平台了。
			{
				warning_report_cmd = 1;
			}			
			break;
		case 2:
			cnt =0;
			len =app_read_his_item(0x19030001,str1,str2,255,0,2000);  //C相过流总次数 抄表的结果应该放在str1里。
			if(!len)
			{
				cnt = (BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			}
			if(cnt !=gSystemInfo.edp_over_flow_C) //如果读回来的告警次数不等于flash中存储的告警次数，那么需要上报平台了。
			{
				warning_report_cmd = 1;
			}
			break;
		case 3:
			cnt =0;
			len =app_read_his_item(0x03300D00,str1,str2,255,0,2000);  //开表盖总次数 抄表的结果应该放在str1里。
			if(!len)
			{
				cnt = (BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			}
			if(cnt !=gSystemInfo.edp_open_meter) //如果读回来的告警次数不等于flash中存储的告警次数，那么需要上报平台了。
			{
				warning_report_cmd = 2;
			}		
			break;
		case 4:
			cnt =0;
			len =app_read_his_item(0x03300100,str1,str2,255,0,2000);  //清零过流总次数 抄表的结果应该放在str1里。
			if(!len)
			{
				cnt = (BCD2byte(str1[2])*10000) + (BCD2byte(str1[1])*100) +(BCD2byte(str1[0]));
			}
			if(cnt !=gSystemInfo.edp_clean_meter) //如果读回来的告警次数不等于flash中存储的告警次数，那么需要上报平台了。
			{
				warning_report_cmd = 3;
			}		
			break;
		default:
			break;
	}
	clr_bit_value(warningMask,1,loop);
	return ;
}

/**
 * 上报告警信息到onenet平台，对平台下发的告警回应进行处理
 */
INT8U report_warning_to_onenet(INT8U flag)
{
	INT8U count =0 ;
	INT32U passMS=0;
	EdpPacket* pkg;
	gSystemInfo.edp_warning_ack =0;
	if(!gSystemInfo.tcp_link) //如果没有登录成功，那么直接存储
	{
#ifdef DEBUG			 
		  	system_debug_info("\r\n***************store hold day energy****************\r\n");
#endif			
//		cjsonEnergyInfoPack(flag,OneNetData);
		cjsonEventWarningPack(OneNetData,flag);
		pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。
		return 1;
	}
	if(GprsObj.send_len)
	{
		return  0 ;
	}
	mem_set(OneNetData,1024, 0x00);  	
//	tcp_client_socket_id =0;   //9600是在前面自己获得的，一般是1
	while(!gSystemInfo.save_ack)
	{
		cjsonEventWarningPack(OneNetData,flag);
		EdpPacket* pkg =PacketSavedataJson2(NULL, OneNetData, 1, 1);
		GprsObj.send_len =pkg->_write_pos;
		GprsObj.send_ptr = pkg->_data;
		gprs_send_app_data();
		passMS =system_get_tick10ms();
		while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
		{

			gprs_read_app_data();
		}
#ifdef DEBUG
		system_debug_info("\r\n*********in report warning info **********");
#endif		
		if(count++>3)
		{
			handleUnsuccessReportDataWrite(pkg->_data,pkg->_write_pos);//如果没上传成功那么存储下来。			
			gSystemInfo.tcp_link = 0;
			gSystemInfo.edp_login_state =0;
			gSystemInfo.dev_config_ack = 0;
			break;		
		}
	}
	return 1;
}

/**
*@brief  handle some message that doesn't report to onenet. write some message to flash.
*随机延时上报，将待上报数据存储到flash，等待时间点到了然后上报。
*/
INT8U tmp[4096] ={0};
void handle_wait_report_data_write(INT8U *buf,INT16U len)
{
	INT8U i=0,has_find =0,find_pos_state =0,find_pos =0,store_flag=0;  //这里store_flag既用于存储标志，又用于擦除块的计算。
	INT64U report_time = 0;
	//********************找到最老的那个需要上传的数据***************************//
	store_flag = 1;
	for(i=0;i<40;)
	{
		has_find = 0;
		switch (find_pos_state)
		{
			case 0:
			if(edpStoreWaitDataState[i++] ==0x01)
			find_pos_state =2;
			break;
			case 2:
			if(edpStoreWaitDataState[i] == 0x00)
			{
				find_pos = i;
				has_find = 1;
			}
			i++;
			if(i==40)
			{
				i=0;
			}
			break;
			default:
			break;
		}
		if(has_find)  //找到了！！！
		{
			break;
		}
	} // 结束for循环
	if(find_pos ==0)  //每次要往第一个块里写的时候，。
	{
//		nor_flash_erase_page(FLASH_EDP_RAND_REPORT_STORE_START);
		#ifdef DEBUG
		system_debug_info("\r\n**************************need to ereaser a new block 0 ****************************\r\n");
		#endif
	}
	if(find_pos%8 == 7) //说明本次存储的位置是每一个sector的最后一块。//存完之后还有将下一个块擦掉
	{
		nor_flash_write_data(FLASH_EDP_RAND_REPORT_STORE_START+find_pos/8,500*(find_pos%8),buf,len);	
		nor_flash_read_data(FLASH_EDP_RAND_REPORT_STORE_START+find_pos/8,500*(find_pos%8),tmp,500);	
		system_debug_info("===================================");
		system_debug_data(tmp,500);
		system_debug_info("===================================");		
		nor_flash_write_data(FLASH_EDP_RAND_REPORT_STORE_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
		mem_cpy((INT8U*)&report_time,buf,8);
		rand_wait_report_time_index[find_pos] =report_time;
		edpStoreWaitDataState[find_pos] = 1;
		if(find_pos==39)
		{
			store_flag = 0;
		}
		else
		{
			store_flag = find_pos/8+1;
		}
		nor_flash_erase_page(FLASH_EDP_RAND_REPORT_STORE_START+store_flag);
		mem_set(edpStoreWaitDataState+8*store_flag,8,0x00);
		#ifdef	DEBUG
		system_debug_info("\r\n**********************unsucessful store data need to change page*******************************\r\n");
		#endif
	}
	else //否则的话直接存储即可
	{
		nor_flash_write_data(FLASH_EDP_RAND_REPORT_STORE_START+find_pos/8,500*(find_pos%8),buf,len);	
		nor_flash_read_data(FLASH_EDP_RAND_REPORT_STORE_START+find_pos/8,500*(find_pos%8),tmp,500);
		system_debug_info("===================================");
		system_debug_data(tmp,500);
		system_debug_info("===================================");		
		nor_flash_write_data(FLASH_EDP_RAND_REPORT_STORE_START+find_pos/8,STORE_RECORD_START + (find_pos%8),&store_flag,1);
		mem_cpy((INT8U*)&report_time,buf,8);
		rand_wait_report_time_index[find_pos] =report_time;
		edpStoreWaitDataState[find_pos] = 1;
		#ifdef	DEBUG
		system_debug_info("\r\n**********************unsucessful store data*******************************\r\n");
		#endif
	}
}
/**
*@brief  handle some message that doesn't report to onenet. read some message from flash.
*将延时上报存储的数据上报到onenet平台，这里只需要跟局全局变量rand_wait_report_time_index 选出需要上报的索引即可。
*/
//INT8U report_buf[500] ={0xFF};
void handle_wait_report_data_read()
{
	INT8U pos =0,page=0,i=0;
	INT8U buftmp[8]={0x0},find_pos_state=0,find_pos=0,has_find =0;
	INT64U store_flag =0;
	INT32U passMS =0;
	INT8U count =0;
	INT16U reportDataLen = 500,idx =0;
	INT8U index=0;
	INT64U report_time =0;
	DateTime dt;
	tpos_datetime(&dt);
	report_time = getPassedSeconds(&dt,2000);
	for(index =0;index<40;index++)
	{
		if((rand_wait_report_time_index[index]<report_time)  &&(rand_wait_report_time_index[index] !=0))
		{
			break;
		}
	}
	if(index ==40)  //在rand_wait_report_time_index 中找了一圈没找到可以上报的，故直接退出。!!!!!!
	{
		return;
	}
		nor_flash_read_data(FLASH_EDP_RAND_REPORT_STORE_START+index/8,500*(index%8),report_buf,500);
		for(idx=500-1;idx>0;idx--)  //得到要回复的报文的长度。
		{
			if(*(report_buf+idx) == 0xff)
			{
				reportDataLen --;
			}
			else
			{
				break;
			}
		}
		if(GprsObj.send_len)
		{
			return  ;
		}
	if(gSystemInfo.tcp_link ==0)  //如果模块不在线，那么就直接存储到未上报区域。
	{
#ifdef DEBUG
		system_debug_info("==================owning outof line ,so store info ======================");
#endif
		handleUnsuccessReportDataWrite(report_buf+8, reportDataLen -8);//如果没上传成功那么存储下来。
		store_flag =0x00;   //store_flag=0 代表该点的数据已经上报了
		nor_flash_write_data(FLASH_EDP_RAND_REPORT_STORE_START+idx/8,500*(index%8),&store_flag,8);
		rand_wait_report_time_index[index] = 0;		
		return;
	}		
		gSystemInfo.save_ack =0;
		while(!gSystemInfo.save_ack)
		{
			GprsObj.send_len = reportDataLen -8;  //考虑到开始8个字节用来存时间
			GprsObj.send_ptr = report_buf+8;     //考虑到开始8个字节用来存时间
			gprs_send_app_data();
			#ifdef DEBUG
			system_debug_info("\r\n*******send ok ******\r\n");
			#endif
			passMS =system_get_tick10ms();
			while((second_elapsed(passMS)<10)&&(!gSystemInfo.save_ack)) //小于10秒
			{

				gprs_read_app_data();
			}
			#ifdef DEBUG
			system_debug_info("\r\n*********in report wait report store info **********");
			#endif
			if(count++>3)
			{

				gSystemInfo.tcp_link = 0;
				gSystemInfo.edp_login_state=0;
				gSystemInfo.dev_config_ack = 0;
				handleUnsuccessReportDataWrite(GprsObj.send_ptr,GprsObj.send_len);//如果没上传成功那么存储下来。				
				break;
				//app_softReset();
			}
		}
		{  //对于按照队列随机延时上报的数据，不管上报成功与否，都需要将其从延时队列flash中除去，方法就是将时间位置清零。在这个逻辑里，我们根据时间的大小进行数据的上报，
			//edpStoreWaitDataState 中的标志位只是用于标志flash的利用情况。即写到了flash哪里了已经。
		store_flag =0x00;   //store_flag=0 代表该点的数据已经上报了
		nor_flash_write_data(FLASH_EDP_RAND_REPORT_STORE_START+idx/8,500*(index%8),&store_flag,8);
		rand_wait_report_time_index[index] = 0;
		}
	return ;
}
//得到随机延时,用表号和时间做异或得到的
INT16U getrand()
{
	INT16U randNum =0;
	INT16U systemtic=system_get_tick10ms();
	tagDatetime time;
	os_get_datetime(&time);
	
	randNum = systemtic^(time.year)^(time.month)^(time.hour)^(time.minute)^(time.second)^(time.msecond_h<<8+time.msecond_l);
	
	randNum =randNum^(gSystemInfo.meter_no[0]+(gSystemInfo.meter_no[1]<<8));
	randNum =randNum^(gSystemInfo.meter_no[2]+(gSystemInfo.meter_no[3]<<8));
	randNum =randNum^(gSystemInfo.meter_no[4]+(gSystemInfo.meter_no[5]<<8));
	return randNum;
}