#pragma once
#include "zboss_decl.h"
#include "commands.h"

extern "C" void zboss_signal_handler(zb_uint8_t param);

class zb_ncp {
public:
	enum frame_type_t : uint8_t {
		REQUEST = 0,
	    RESPONSE = 1,
	    INDICATION = 2,
	};
	struct cmd_t {
		uint8_t version;
		frame_type_t type;
		command_id_t command_id;
		uint8_t tsn;
	} __attribute__((packed));
	static constexpr size_t MAX_PARALLEL_REQUESTS = 16;
	static constexpr size_t ZB_TASK_STACK_SIZE = 1024 * 8;
private:
	template <command_id_t Cmd>
	struct cmd_handle;
	template <command_id_t CmdId>
	struct immediate_cmd_process;
	template <command_id_t CmdId,template<command_id_t> typename ResolveStrategyT>
	struct delayed_cmd_process;
	template <command_id_t CmdId,typename Arg, typename Req, typename Resp>
	struct request_cmd_process;
	template <command_id_t CmdId,typename Res>
	struct general_status_res;
	template <command_id_t CmdId,typename Arg>
	struct general_status_arg;
	template <command_id_t CmdId,typename Arg,typename Res>
	struct general_status_arg_res;
	
	friend void zboss_signal_handler(zb_uint8_t param);

	uint32_t m_channels_mask;
	static void continue_zboss(uint8_t );
	static void set_channel_mask(uint32_t mask);
	static bool start_zigbee_stack();
	static void ncp_zb_task(void* arg);

private:
	zb_ncp();
	static zb_ncp& instance();
	esp_err_t init_int();
public:
	static esp_err_t init() { return instance().init_int(); }
	static void on_rx_data(const void* data,size_t size);
	static void indication(command_id_t cmd,const void* data,size_t size);
	static void send_cmd_data(const void* data,size_t size);
};