#include "pft.h"


struct priv {
	sap_agent_h agent;
	sap_peer_agent_h peer_agent;
	sap_file_transaction_h file_socket;
};

static struct priv ft_priv_data = { 0 };
//struct priv ft_priv_data;
static int connected = 0;


static void ft_on_send_completed(sap_file_transaction_h file_transaction, sap_ft_transfer_e result, const char *file_path, void *user_data)
{
	char error_message[100];

	if (ft_priv_data.file_socket) {
		sap_file_transfer_destroy(ft_priv_data.file_socket);
	}

	ft_priv_data.file_socket = NULL;

	if (result == SAP_FT_TRANSFER_SUCCESS) {
		sprintf(error_message, "Transfer Completed");
		dlog_print(DLOG_INFO, LOG_TAG, ">>%s", error_message);

		ui_app_exit();
	} else {
		switch (result) {
		case (SAP_FT_TRANSFER_FAIL_CHANNEL_IO): {
			sprintf(error_message, "Channel IO Error.");
			break;
		}

		case (SAP_FT_TRANSFER_FAIL_FILE_IO): {
			sprintf(error_message, "File IO Error.");
			break;
		}

		case (SAP_FT_TRANSFER_FAIL_CMD_DROPPED): {
			sprintf(error_message, "Transfer dropped/");
			break;
		}

		case (SAP_FT_TRANSFER_FAIL_PEER_UNRESPONSIVE): {
			sprintf(error_message, "Peer Un Responsive.");
			break;
		}

		case (SAP_FT_TRANSFER_FAIL_PEER_CONN_LOST): {
			sprintf(error_message, "Device Connection Lost.");
			break;
		}

		case (SAP_FT_TRANSFER_FAIL_PEER_CANCELLED): {
			sprintf(error_message, "Peer Cancelled.");
			break;
		}

		case (SAP_FT_TRANSFER_FAIL_SPACE_NOT_AVAILABLE): {
			sprintf(error_message, "No Space.");
			break;
		}

		default:
			sprintf(error_message, "Unknown Error");
		}

		dlog_print(DLOG_INFO, LOG_TAG, ">>%s", error_message);
	}
}

static void ft_on_sending_file_in_progress(sap_file_transaction_h file_transaction, unsigned short int percentage_progress, void *user_data)
{
	dlog_print(DLOG_INFO, LOG_TAG, ">> on_file_progress: Trans:%d, Progress:%d\n", file_transaction, percentage_progress);
}



void ft_send_file()
{
	sap_peer_agent_h pa = ft_priv_data.peer_agent;

	dlog_print(DLOG_INFO, LOG_TAG, ">> sending file");
	char filename[256] = {'\0'};
	char *path;
	storage_get_directory(STORAGE_TYPE_INTERNAL, STORAGE_DIRECTORY_SOUNDS, &path);
	snprintf(filename, sizeof(filename), "%s%s", path, "/voice.wav");
	dlog_print(DLOG_INFO, LOG_TAG, ">> %s", filename);

	char file_path[100];

	sprintf(file_path, "%svoice.wav", app_get_shared_resource_path());
	dlog_print(DLOG_INFO, LOG_TAG, ">> %s", file_path);

	int ret = sap_file_transfer_send(ft_priv_data.peer_agent, file_path, &ft_priv_data.file_socket);
	dlog_print(DLOG_INFO, LOG_TAG, ">> sap_file_transfer_send: %d", ret);
	dlog_print(DLOG_INFO, LOG_TAG, ">> %d,%d,%d", SAP_RESULT_PERMISSION_DENIED, SAP_RESULT_FAILURE, SAP_RESULT_SUCCESS);

	ret = sap_file_transfer_set_progress_cb(ft_priv_data.file_socket, ft_on_sending_file_in_progress, NULL);
	dlog_print(DLOG_INFO, LOG_TAG, ">> sap_file_transfer_set_progress_cb: %d", ret);

	ret = sap_file_transfer_set_done_cb(ft_priv_data.file_socket, ft_on_send_completed, NULL);
	dlog_print(DLOG_INFO, LOG_TAG, ">> sap_file_transfer_set_done_cb: %d", ret);

	dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] SET callbacks for socket :%u", ft_priv_data.file_socket);

	if (ft_priv_data.file_socket == NULL) {
		dlog_print(DLOG_INFO, LOG_TAG, ">> some error :(");
		return;
	}

	dlog_print(DLOG_INFO, LOG_TAG, ">> Seems ok");
}


void ft_on_peer_agent_updated(sap_peer_agent_h peer_agent, sap_peer_agent_status_e peer_status, sap_peer_agent_found_result_e result, void *user_data)
{
	switch (result) {
	case SAP_PEER_AGENT_FOUND_RESULT_DEVICE_NOT_CONNECTED:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] device is not connected");
		update_ui("Device is not connected yet.");
		break;

	case SAP_PEER_AGENT_FOUND_RESULT_FOUND:

		if (peer_status == SAP_PEER_AGENT_STATUS_AVAILABLE) {
			ft_priv_data.peer_agent = peer_agent;
			update_ui("peer found. Now u can send data.");
		} else {
			sap_peer_agent_destroy(peer_agent);
			update_ui("peer lost.");
			ft_priv_data.peer_agent = NULL;
		}

		break;

	case SAP_PEER_AGENT_FOUND_RESULT_SERVICE_NOT_FOUND:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] service not found");
		update_ui("peer not found.");
		break;

	case SAP_PEER_AGENT_FOUND_RESULT_TIMEDOUT:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] peer agent find timed out");
		update_ui("find peer timed out");
		break;

	case SAP_PEER_AGENT_FOUND_RESULT_INTERNAL_ERROR:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] peer agent find search failed");
		update_ui("find peer sap error");
		break;
	}
}


static gboolean ft_find_peer_agent()
{
	sap_result_e result = SAP_RESULT_FAILURE;

	result = sap_agent_find_peer_agent(ft_priv_data.agent, ft_on_peer_agent_updated, NULL);

	if (result == SAP_RESULT_SUCCESS) {
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] find peer call succeeded");
	} else {
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] findsap_peer_agent_s is failed (%d)", result);
	}
	dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] find peer call is over");
	return FALSE;
}

gboolean ft_find_peers()
{
	ft_find_peer_agent();
	return TRUE;
}

void cancel_file()
{
	sap_file_transfer_cancel(ft_priv_data.file_socket);
}

static void ft_on_agent_initialized(sap_agent_h agent, sap_agent_initialized_result_e result, void *user_data)
{
	switch (result) {
	case SAP_AGENT_INITIALIZED_RESULT_SUCCESS:
		dlog_print(DLOG_INFO, LOG_TAG, "agent is initialized");

		ft_priv_data.agent = agent;
		ft_find_peers();
		break;

	case SAP_AGENT_INITIALIZED_RESULT_DUPLICATED:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] duplicate registration");
		break;

	case SAP_AGENT_INITIALIZED_RESULT_INVALID_ARGUMENTS:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] invalid arguments");
		break;

	case SAP_AGENT_INITIALIZED_RESULT_INTERNAL_ERROR:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] internal sap error");
		break;

	default:
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] unknown status (%d)", result);
		break;
	}

	dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] agent initialized callback is over");

}



static void ft_on_device_status_changed(sap_device_status_e status, sap_transport_type_e transport_type, void *user_data)
{

	connected = status;

	switch (transport_type) {
		case SAP_TRANSPORT_TYPE_BT:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] transport_type (%d): bt", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_BLE:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] transport_type (%d): ble", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_TCP:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] transport_type (%d): tcp/ip", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_USB:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] transport_type (%d): usb", transport_type);
			break;

		case SAP_TRANSPORT_TYPE_MOBILE:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] transport_type (%d): mobile", transport_type);
			break;

		default:
			dlog_print(DLOG_ERROR, LOG_TAG, "[FT] unknown transport_type (%d)", transport_type);
			break;
	}

	switch (status) {
		case SAP_DEVICE_STATUS_DETACHED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] DEVICE GOT DISCONNECTED");
			sap_peer_agent_destroy(ft_priv_data.peer_agent);
			ft_priv_data.peer_agent = NULL;
			update_ui("[FT] Device Disconnected. Call find peer after reconnection.");
			break;

		case SAP_DEVICE_STATUS_ATTACHED:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] DEVICE IS CONNECTED NOW, PLEASE CALL FIND PEER");
			update_ui("[FT] Device Connected. Call Find Peer.");
			break;

		default:
			dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] unknown status (%d)", status);
			break;
	}
}


gboolean ft_agent_initialize()
{
	int result = 0;

	do {
		result = sap_agent_initialize(ft_priv_data.agent, FT_PROFILE_ID, SAP_AGENT_ROLE_CONSUMER, ft_on_agent_initialized, NULL);
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] SAP >>> getRegisteredServiceAgent() >>> %d", result);
	} while (result != SAP_RESULT_SUCCESS);

	return TRUE;
}

void ft_initialize_sap()
{
	sap_agent_h agent = NULL;

	sap_agent_create(&agent);

	if (agent == NULL)
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] ERROR in creating agent");
	else
		dlog_print(DLOG_DEBUG, LOG_TAG, "[FT] successfully created sap agent");

	ft_priv_data.agent = agent;

	sap_set_device_status_changed_cb(ft_on_device_status_changed, NULL);

	ft_agent_initialize();
}
