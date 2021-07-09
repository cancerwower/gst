#include <stdio.h>
#include <string.h>
#include <fstream>
#include <unistd.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

#define BUFF_SIZE (1024)

typedef struct {
	GstPipeline *pipeline;
	GstAppSrc  *src;
	GstElement *filter1;
	GstElement *filter2;
	GstElement *encoder;
	GstElement *filter3;
	GstElement *sink;

	GstClockTime timestamp;
	guint sourceid;
} gst_app_t;

static gst_app_t gst_app;

int main()
{
	gst_app_t *app = &gst_app;
	GstStateChangeReturn state_ret;
	gst_init(NULL, NULL); //Initialize Gstreamer
	app->timestamp = 0; //Set timestamp to 0

	//Create pipeline, and pipeline elements
	app->pipeline = (GstPipeline*)gst_pipeline_new("mypipeline");
	app->src    =   (GstAppSrc*)gst_element_factory_make("appsrc", "mysrc");
	app->filter1 =  gst_element_factory_make ("capsfilter", "myfilter1");
	app->filter2 =  gst_element_factory_make ("videoconvert", "myfilter2");
	app->encoder =  gst_element_factory_make ("omxh264enc", "myomx");
	app->filter3 =  gst_element_factory_make ("capsfilter", "myfilter3");
	app->sink =     gst_element_factory_make ("filesink"  , NULL);
	
	if(	!app->pipeline || 
		!app->src      || !app->filter1 || 
		!app->encoder  || !app->filter2 ||  
		!app->filter3 || 
		!app->sink    )  {
		printf("Error creating pipeline elements!\n");
		exit(2);
	}

	//Attach elements to pipeline
	gst_bin_add_many(
		GST_BIN(app->pipeline), 
		(GstElement*)app->src,
		app->filter1,	
		app->filter2,
		app->encoder,
		app->filter3,
		app->sink,
		NULL);

	//Set pipeline element attributes
	g_object_set (app->src, "format", GST_FORMAT_DEFAULT, NULL);
	g_object_set (app->src, "blocksize", 4147200, NULL);
	GstCaps *filtercaps1 = gst_caps_new_simple ("video/x-raw",
		"format", G_TYPE_STRING, "UYVY",
		"width", G_TYPE_INT, 1920,
		"height", G_TYPE_INT, 1080,
		"framerate", GST_TYPE_FRACTION, 30, 1,
		NULL);
	g_object_set (G_OBJECT (app->filter1), "caps", filtercaps1, NULL);
	GstCaps *filtercaps3 = gst_caps_new_simple ("video/x-h264",
		"stream-format", G_TYPE_STRING, "byte-stream",
		NULL);*/
	g_object_set (G_OBJECT (app->filter3), "caps", filtercaps3, NULL);
	g_object_set (G_OBJECT (app->sink), "location", "output.ts", NULL);

	//Link elements together
	g_assert( gst_element_link_many(
		(GstElement*)app->src, 
		app->filter1,
		app->filter2,
		app->encoder,
		app->filter3,
		app->sink,
		NULL ) );

	//Play the pipeline
	state_ret = gst_element_set_state((GstElement*)app->pipeline, GST_STATE_PLAYING);
	g_assert(state_ret == GST_STATE_CHANGE_ASYNC);

	//Get a pointer to the test input
	FILE *testfile = fopen("test.yuv", "rb");	
	g_assert(testfile != NULL);

	//Push the data from buffer to gstpipeline 100 times
	//for(int i = 0; i < 100; i++) {
	char* filebuffer = (char*)malloc (1920*1080*2*30*10); //Allocate memory for framebuffer
	if (filebuffer == NULL) {printf("Memory error\n"); exit (2);} //Errorcheck
	size_t bytesread = fread(filebuffer, 1 , (1920*1080*2*30*10), testfile); //Read to filebuffer
	//printf("File Read: %zu bytes\n", bytesread);

	GstBuffer *pushbuffer; //Actual databuffer
	GstFlowReturn ret; //Return value
	pushbuffer = gst_buffer_new_wrapped (filebuffer, 1920*1080*2*30*10); //Wrap the data

	//Set frame timestamp
	/*GST_BUFFER_PTS      (pushbuffer) = app->timestamp;
	GST_BUFFER_DTS      (pushbuffer) = app->timestamp;	
	GST_BUFFER_DURATION (pushbuffer) = gst_util_uint64_scale_int (1, GST_SECOND, 1);
	app->timestamp += GST_BUFFER_DURATION (pushbuffer);*/
	//printf("Frame is at %lu\n", app->timestamp);

	ret = gst_app_src_push_buffer( app->src, pushbuffer); //Push data into pipeline

	g_assert(ret ==  GST_FLOW_OK);
	//}
	
	//Declare end of stream
	gst_app_src_end_of_stream (GST_APP_SRC (app->src));
	printf("End Program.\n");
	usleep(100000);
	return 0;
}
