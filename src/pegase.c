#include "pegase.h"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *label;
	recorder_h g_recorder;
	Evas_Object *naviframe;
	Evas_Object *rect[10];
	Eext_Circle_Surface *circle_surface;
	Evas_Object *circle_genlist;
	player_h player;

} appdata_s;


typedef struct _item_data {
	int index;
	Elm_Object_Item *item;
} item_data;

char *main_menu_names[] = {
	"Find Peer", "Fetch",
	NULL
};


bool permission_granted = false;
int pgas_user_clicked = 0;


static void
snd_play(appdata_s *ad)
{
    char audio_path[100];
    sprintf(audio_path, "%send_of_input.wav", app_get_shared_resource_path());
    wav_player_start(audio_path, SOUND_TYPE_VOICE, NULL, NULL, NULL);
}



void update_ui(char *data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Updating UI with data %s", data);
}

static void
win_delete_request_cb(void *data, Evas_Object *obj, void *event_info)
{
	ui_app_exit();
}

static void
win_back_cb(void *data, Evas_Object *obj, void *event_info)
{
	appdata_s *ad = data;
	/* Let window go to hide state. */
	elm_win_lower(ad->win);
}

static void
_state_changed_cb(recorder_state_e previous, recorder_state_e current, bool by_policy, void *user_data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "_recorder_state_changed_cb (prev: %d, curr: %d)\n", previous, current);
}

static void
_recorder_recording_limit_reached_cb(recorder_recording_limit_type_e type, void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG, "Recording limit reached: %d\n", type);
}

/* Check the audio recorder state */
static bool
_recorder_expect_state(recorder_h recorder, recorder_state_e expected_state)
{
    recorder_state_e state;
    int error_code = recorder_get_state(recorder, &state);

    dlog_print(DLOG_INFO, LOG_TAG, "recorder state = %d, expected recorder state = %d", state, expected_state);
    if (state == expected_state)
        return true;

    return false;
}

static char *_gl_title_text_get(void *data, Evas_Object *obj, const char *part)
{
	char buf[1024];

	snprintf(buf, 1023, "%s", "HelloMessage");

	return strdup(buf);
}

static char *_gl_sub_title_text_get(void *data, Evas_Object *obj, const char *part)
{
	char buf[1024];

	snprintf(buf, 1023, "%s", "Consumer");

	return strdup(buf);
}

static char *_gl_main_text_get(void *data, Evas_Object *obj, const char *part)
{
	char buf[1024];
	item_data *id = data;
	int index = id->index;

	if (!strcmp(part, "elm.text"))
		snprintf(buf, 1023, "%s", main_menu_names[index - 1]);

	return strdup(buf);


}
static void _gl_del(void *data, Evas_Object *obj)
{
	// FIXME: Unrealized callback can be called after this.
	// Accessing Item_Data can be dangerous on unrealized callback.
	item_data *id = data;
	if (id) free(id);
}

static Eina_Bool _naviframe_pop_cb(void *data, Elm_Object_Item *it)
{
	ui_app_exit();
	return EINA_FALSE;
}

static void btn_cb_find_peers(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);

	dlog_print(DLOG_DEBUG, LOG_TAG, "AGENT_INITIALISED");
	find_peers();
	ft_find_peers();
}

static void btn_cb_send(void *data, Evas_Object *obj, void *event_info)
{
	Elm_Object_Item *it = event_info;
	elm_genlist_item_selected_set(it, EINA_FALSE);
	mex_send("Hello Message", 9, TRUE);
	ft_send_file();
}


static void create_list_view(appdata_s *ad)
{
	Evas_Object *genlist = NULL;
	Evas_Object *naviframe = ad->naviframe;
	Elm_Object_Item *nf_it = NULL;
	item_data *id = NULL;
	int index = 0;

	Elm_Genlist_Item_Class *itc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *titc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *pitc = elm_genlist_item_class_new();
	Elm_Genlist_Item_Class *gic = elm_genlist_item_class_new();

	/* Genlist Item Style */
	itc->item_style = "1text";
	itc->func.text_get = _gl_main_text_get;
	itc->func.del = _gl_del;

	/* Genlist Title Item Style */
	titc->item_style = "title";
	titc->func.text_get = _gl_title_text_get;
	titc->func.del = _gl_del;

	gic->item_style = "groupindex";
	gic->func.text_get = _gl_sub_title_text_get;
	gic->func.del = _gl_del;

	pitc->item_style = "padding";

	/* Create Genlist */
	genlist = elm_genlist_add(naviframe);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	evas_object_smart_callback_add(genlist, "selected", NULL, NULL);

	/* Create Circle Genlist */
	ad->circle_genlist = eext_circle_object_genlist_add(genlist, ad->circle_surface);

	/* Set Scroller Policy */
	eext_circle_object_genlist_scroller_policy_set(ad->circle_genlist, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

	/* Activate Rotary Event */
	eext_rotary_object_event_activated_set(ad->circle_genlist, EINA_TRUE);

	/* Title Item Here */
	id = calloc(sizeof(item_data), 1);
	elm_genlist_item_append(genlist, titc, NULL, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	id = calloc(sizeof(item_data), 1);
	id->index = index++;
	id->item = elm_genlist_item_append(genlist, gic, id, NULL, ELM_GENLIST_ITEM_GROUP, NULL, NULL);

	/* Main Menu Items Here*/
	id = calloc(sizeof(item_data), 1);
	id->index = index++;
	id->item = elm_genlist_item_append(genlist, itc, id, NULL, ELM_GENLIST_ITEM_NONE, btn_cb_find_peers, ad);
	id = calloc(sizeof(item_data), 1);
	id->index = index++;
	id->item = elm_genlist_item_append(genlist, itc, id, NULL, ELM_GENLIST_ITEM_NONE, btn_cb_send, ad);

	elm_genlist_item_append(genlist, pitc, NULL, NULL, ELM_GENLIST_ITEM_NONE, NULL, ad);

	nf_it = elm_naviframe_item_push(naviframe, NULL, NULL, NULL, genlist, "empty");
	elm_naviframe_item_pop_cb_set(nf_it, _naviframe_pop_cb, ad->win);


}


static void
create_base_gui(appdata_s *ad)
{
	/* Window */
	/* Create and initialize elm_win.
	   elm_win is mandatory to manipulate window. */
	ad->win = elm_win_util_standard_add(PACKAGE, PACKAGE);
	elm_win_autodel_set(ad->win, EINA_TRUE);

	if (elm_win_wm_rotation_supported_get(ad->win)) {
		int rots[4] = { 0, 90, 180, 270 };
		elm_win_wm_rotation_available_rotations_set(ad->win, (const int *)(&rots), 4);
	}

	evas_object_smart_callback_add(ad->win, "delete,request", win_delete_request_cb, NULL);
	eext_object_event_callback_add(ad->win, EEXT_CALLBACK_BACK, win_back_cb, ad);

	/* Conformant */
	/* Create and initialize elm_conformant.
	   elm_conformant is mandatory for base gui to have proper size
	   when indicator or virtual keypad is visible. */
	ad->conform = elm_conformant_add(ad->win);
	elm_win_indicator_mode_set(ad->win, ELM_WIN_INDICATOR_SHOW);
	elm_win_indicator_opacity_set(ad->win, ELM_WIN_INDICATOR_OPAQUE);
	evas_object_size_hint_weight_set(ad->conform, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_win_resize_object_add(ad->win, ad->conform);
	evas_object_show(ad->conform);

	/* Label */
	/* Create an actual view of the base gui.
	   Modify this part to change the view. */
	ad->label = elm_label_add(ad->conform);
	elm_object_text_set(ad->label, "<align=center>Hello Tizen</align>");

	evas_object_size_hint_weight_set(ad->label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_object_content_set(ad->conform, ad->label);


	/* Naviframe */
	//ad->naviframe = elm_naviframe_add(ad->conform);
	//elm_object_content_set(ad->conform, ad->naviframe);

	/* Eext Circle Surface*/
	//ad->circle_surface = eext_circle_surface_naviframe_add(ad->naviframe);

	/* Main View */
	//create_list_view(ad);

	eext_object_event_callback_add(ad->naviframe, EEXT_CALLBACK_BACK, eext_naviframe_back_cb, NULL);
	eext_object_event_callback_add(ad->naviframe, EEXT_CALLBACK_MORE, eext_naviframe_more_cb, NULL);

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}



static Eina_Bool
_timer2_cb(appdata_s *ad)
{
	dlog_print(DLOG_INFO, LOG_TAG, "CALLBACK TIMEOUT !");
	elm_object_text_set(ad->label, "<align=center>WILL END</align>");

	recorder_state_e state;
	int error_code = recorder_get_state(ad->g_recorder, &state);


	char str[80]; sprintf(str, "<align=center>z: %d,%d,%d</align>", state, RECORDER_STATE_RECORDING, RECORDER_STATE_PAUSED );
	elm_object_text_set(ad->label, str);

	// Stop the recorder and save the recorded data to a file
	if (_recorder_expect_state(ad->g_recorder, RECORDER_STATE_RECORDING) || _recorder_expect_state(ad->g_recorder, RECORDER_STATE_PAUSED)) {
		int error_code = recorder_commit(ad->g_recorder);

		if (error_code != RECORDER_ERROR_NONE) {
			dlog_print(DLOG_ERROR, LOG_TAG, "error code = %d", error_code);
			elm_object_text_set(ad->label, "<align=center>END ERR</align>");
		} else {
			elm_object_text_set(ad->label, "<align=center>END OK</align>");
		}

		error_code = recorder_unset_recording_limit_reached_cb(ad->g_recorder);
	    error_code = recorder_unprepare(ad->g_recorder);
	    error_code = recorder_unset_state_changed_cb(ad->g_recorder);
	    error_code = recorder_destroy(ad->g_recorder);
	}

	timer2 = NULL;

	ft_send_file();

	return ECORE_CALLBACK_CANCEL;
}


static bool
_go_listen(appdata_s *ad) {
	/* Create the audio recorder handle */
	int error_code = recorder_create_audiorecorder(&ad->g_recorder);
	if (error_code != RECORDER_ERROR_NONE)
		dlog_print(DLOG_ERROR, LOG_TAG, "fail to create an Audio Recorder: error code = %d", error_code);

	error_code = recorder_set_state_changed_cb(ad->g_recorder, _state_changed_cb, NULL);
	error_code = recorder_set_recording_limit_reached_cb(ad->g_recorder, _recorder_recording_limit_reached_cb, NULL);

	struct tm localtime = {0};
	time_t rawtime = time(NULL);
	char filename[256] = {'\0'};
	size_t size;

	/* Set the audio encoder */
	error_code = recorder_set_audio_encoder(ad->g_recorder, RECORDER_AUDIO_CODEC_PCM);
	error_code = recorder_set_file_format(ad->g_recorder, RECORDER_FILE_FORMAT_WAV);

	/* Create the file name */
	 /*" /opt/usr/home/owner/media/Sounds/"*/

	if (localtime_r(&rawtime, &localtime) != NULL) {
		/*
		size = snprintf(filename, sizeof(filename), "%s%s_%04i_%02i_%02i_%02i_%02i_%02i.3gp",
						app_get_data_path(), FILENAME_PREFIX,
						localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
						localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
		*/

		char *path;
		storage_get_directory(STORAGE_TYPE_INTERNAL, STORAGE_DIRECTORY_SOUNDS, &path);



		size = snprintf(filename, sizeof(filename), "%s%s", path, "/voice.wav");
	} else {
		/* Error handling */
	}

	char file_path[100];

	sprintf(file_path, "%svoice.wav", app_get_shared_resource_path());

	// /opt/usr/home/owner/media/Sounds
	dlog_print(DLOG_INFO, LOG_TAG, "%s", file_path);

	/* Set the full path and file name */
	/* Set the file name according to the file format */
	error_code = recorder_set_filename(ad->g_recorder,  file_path);

	/* Set the maximum file size to 1024 (kB) */
	//error_code = recorder_attr_set_size_limit(ad->g_recorder, 2048);
	error_code = recorder_attr_set_time_limit(ad->g_recorder, RECORD_LIMIT + 1);

	/* Set the audio encoder bitrate */
	error_code = recorder_attr_set_audio_encoder_bitrate(ad->g_recorder, 16000);

	error_code = recorder_attr_set_audio_channel(ad->g_recorder, 1);

	/* Set the audio device to microphone */
	error_code = recorder_attr_set_audio_device(ad->g_recorder, RECORDER_AUDIO_DEVICE_MIC);

	/* Set the audio sample rate */
	error_code = recorder_attr_set_audio_samplerate(ad->g_recorder, 16000);


	error_code = recorder_prepare(ad->g_recorder);

	error_code = recorder_start(ad->g_recorder);

	elm_object_text_set(ad->label, "<align=center>START</align>");
	snd_play(ad);

	/*
	recorder_state_e state;
	recorder_get_state(ad->g_recorder, &state);
	dlog_print(DLOG_INFO, LOG_TAG, "%d and %d", state, error_code);

	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_NONE                );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_INVALID_PARAMETER   );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_INVALID_STATE       );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_OUT_OF_MEMORY       );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_DEVICE              );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_INVALID_OPERATION   );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_SOUND_POLICY        );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_SECURITY_RESTRICTED );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_SOUND_POLICY_BY_CALL);
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_SOUND_POLICY_BY_ALARM);
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_ESD                 );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_OUT_OF_STORAGE      );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_PERMISSION_DENIED   );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_NOT_SUPPORTED       );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_RESOURCE_CONFLICT   );
	dlog_print(DLOG_INFO, LOG_TAG, "%d", RECORDER_ERROR_SERVICE_DISCONNECTED);
	*/

	timer2 = ecore_timer_add(RECORD_LIMIT, _timer2_cb, ad);

	return true;
}

static bool
app_create(void *data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "app_create");
	/* Hook to take necessary actions before main event loop starts
		Initialize UI resources and application's data
		If this function returns true, the main loop of application starts
		If this function returns false, the application is terminated */
	appdata_s *ad = data;

	create_base_gui(ad);

	elm_object_text_set(ad->label, "<align=center>APP CREATE</align>");


	dlog_print(DLOG_INFO, LOG_TAG, "SAP INIT");

	//initialize_sap();
	ft_initialize_sap();

	return true;
}

static void
app_control(app_control_h app_control, void *data)
{
	/* Handle the launch request. */
	dlog_print(DLOG_INFO, LOG_TAG, "app_control");
}

static void
app_pause(void *data)
{
	/* Take necessary actions when application becomes invisible. */
	dlog_print(DLOG_INFO, LOG_TAG, "app_pause");
}

static void permission_request_cb(ppm_call_cause_e cause, ppm_request_result_e result, const char *privilege, void *user_data)
{
    dlog_print(DLOG_INFO, LOG_TAG, "callback called for privilege: %s with cause: %d, the result was: %d", privilege, cause, result);
    pgas_user_clicked ++;
}

void *_waiting_for_privileges(appdata_s *ad) {
    while (pgas_user_clicked  < NUMBER_OF_PRIVILEGES)
    {
        dlog_print(DLOG_INFO, LOG_TAG, "waiting for user to accept all privileges, so far accepted: %d", pgas_user_clicked );
		elm_object_text_set(ad->label, "<align=center>PRIVI !</align>");
        sleep(1);
    }
	
	_go_listen(ad);
	
    return 0;
}

static void
app_resume(void *data)
{
	/* Take necessary actions when application becomes visible. */
	dlog_print(DLOG_INFO, LOG_TAG, "app_resume");
	
	appdata_s *ad = data;


	ppm_check_result_e privilege_results_array[NUMBER_OF_PRIVILEGES];
	const char *privilege_array[NUMBER_OF_PRIVILEGES];
	pthread_t pid;
	char *recorder_privilege = "http://tizen.org/privilege/recorder";
	char *media_storage_privilege = "http://tizen.org/privilege/mediastorage";
	char *ex_media_storage_privilege = "http://tizen.org/privilege/externalstorage";

	privilege_array[0] = malloc(strlen(recorder_privilege) + 1);
	privilege_array[1] = malloc(strlen(media_storage_privilege) + 1);
	privilege_array[2] = malloc(strlen(ex_media_storage_privilege) + 1);

	strcpy((char*) privilege_array[0], recorder_privilege);
	strcpy((char*) privilege_array[1], media_storage_privilege);
	strcpy((char*) privilege_array[2], ex_media_storage_privilege);

	int allowed = 0;

	for (int i = 0; i < NUMBER_OF_PRIVILEGES; i++)
	{
		int result = ppm_check_permission(privilege_array[i], &privilege_results_array[i]);
		dlog_print(DLOG_INFO, LOG_TAG, "checking permission '%s' for recorder. Result: %d", privilege_array[i], result);

		switch (privilege_results_array[i])
		{
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ALLOW:
			dlog_print(DLOG_INFO, LOG_TAG, "Privilege: %s, Allowed!", privilege_array[i]);
			allowed++;
			break;
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_DENY:
			dlog_print(DLOG_INFO, LOG_TAG, "Privilege: %s, Denied!", privilege_array[i]);
			break;
		case PRIVACY_PRIVILEGE_MANAGER_CHECK_RESULT_ASK:
			dlog_print(DLOG_INFO, LOG_TAG, "Privilege: %s, Gotta ask for this privilege!", privilege_array[i]);
			ppm_request_permission(privilege_array[i], permission_request_cb, NULL);
			break;
		}
	}


	if (allowed < NUMBER_OF_PRIVILEGES) {
		pthread_create(&pid, NULL, _waiting_for_privileges, ad);
	} else {
		_go_listen(ad);
	}
}

static void
app_terminate(void *data)
{
	/* Release all resources. */
	dlog_print(DLOG_INFO, LOG_TAG, "app_terminate");
	
	appdata_s *ad = data;
	
	int error_code = recorder_unset_recording_limit_reached_cb(ad->g_recorder);
    error_code = recorder_unprepare(ad->g_recorder);
    error_code = recorder_unset_state_changed_cb(ad->g_recorder);
    error_code = recorder_destroy(ad->g_recorder);
	
	if (error_code != RECORDER_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "fail to destroy recorder: error code = %d", error_code);
	}
}

static void
ui_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	char *locale = NULL;
	system_settings_get_value_string(SYSTEM_SETTINGS_KEY_LOCALE_LANGUAGE, &locale);
	elm_language_set(locale);
	free(locale);
	return;
}

static void
ui_app_orient_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_DEVICE_ORIENTATION_CHANGED*/
	return;
}

static void
ui_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
ui_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
ui_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}




int
main(int argc, char *argv[])
{
	appdata_s ad = {0,};
	int ret = 0;

	ui_app_lifecycle_callback_s event_callback = {0,};
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = app_create;
	event_callback.terminate = app_terminate;
	event_callback.pause = app_pause;
	event_callback.resume = app_resume;
	event_callback.app_control = app_control;

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, ui_app_low_battery, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, ui_app_low_memory, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED], APP_EVENT_DEVICE_ORIENTATION_CHANGED, ui_app_orient_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, ui_app_lang_changed, &ad);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, ui_app_region_changed, &ad);

	ret = ui_app_main(argc, argv, &event_callback, &ad);
	if (ret != APP_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "app_main() is failed. err = %d", ret);
	}

	return ret;
}
