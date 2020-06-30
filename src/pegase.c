#include "pegase.h"

typedef struct appdata {
	Evas_Object *win;
	Evas_Object *conform;
	Evas_Object *label;
	recorder_h g_recorder;
} appdata_s;

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

	/* Show window after base gui is set up */
	evas_object_show(ad->win);
}



static Eina_Bool
_timer2_cb(appdata_s *ad)
{
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
	}

	timer2 = NULL;
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
	error_code = recorder_set_audio_encoder(ad->g_recorder, RECORDER_AUDIO_CODEC_AAC);
	error_code = recorder_set_file_format(ad->g_recorder, RECORDER_FILE_FORMAT_3GP);

	/* Create the file name */
	if (localtime_r(&rawtime, &localtime) != NULL) {
		size = snprintf(filename, sizeof(filename), "%s%s_%04i_%02i_%02i_%02i_%02i_%02i.3gp",
						app_get_data_path(), FILENAME_PREFIX,
						localtime.tm_year + 1900, localtime.tm_mon + 1, localtime.tm_mday,
						localtime.tm_hour, localtime.tm_min, localtime.tm_sec);
	} else {
		/* Error handling */
	}

	// /opt/usr/home/owner/media/Sounds
	dlog_print(DLOG_INFO, LOG_TAG, "%s", filename);

	/* Set the full path and file name */
	/* Set the file name according to the file format */
	error_code = recorder_set_filename(ad->g_recorder,  filename);

	/* Set the maximum file size to 1024 (kB) */
	error_code = recorder_attr_set_size_limit(ad->g_recorder, 1024);

	/* Set the audio encoder bitrate */
	error_code = recorder_attr_set_audio_encoder_bitrate(ad->g_recorder, 28800);

	/* Set the audio device to microphone */
	error_code = recorder_attr_set_audio_device(ad->g_recorder, RECORDER_AUDIO_DEVICE_MIC);

	/* Set the audio sample rate */
	error_code = recorder_attr_set_audio_samplerate(ad->g_recorder, 44100);


	error_code = recorder_prepare(ad->g_recorder);

	error_code = recorder_start(ad->g_recorder);

	elm_object_text_set(ad->label, "<align=center>START</align>");

	recorder_state_e state;
	int aa = recorder_get_state(ad->g_recorder, &state);
	dlog_print(DLOG_INFO, LOG_TAG, "%d and %d", state, error_code);

	//timer2 = ecore_timer_add(3.0, _timer2_cb, ad);

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
    user_clicked++;
}

void *_waiting_for_privileges(appdata_s *ad)
{
    while (user_clicked < NUMBER_OF_PRIVILEGES)
    {
        dlog_print(DLOG_INFO, LOG_TAG, "waiting for user to accept all privileges, so far accepted: %d", user_clicked);
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
