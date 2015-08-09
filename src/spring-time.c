#include <pebble.h>

#define MATH_PI 3.141592653589793238462
#define NUM_DISCS 3
#define DISC_DENSITY 0.05
#define ACCEL_RATIO 0.5
#define ACCEL_STEP_MS 40

typedef struct Vec2d {
	double x;
	double y;
} Vec2d;

typedef struct Disc {
	GColor color;
	Vec2d pos;
	Vec2d vel;
	double mass;
	double radius;
} Disc;

static Window *s_main_window;
static Layer *s_disc_layer;
static Layer *display_layer;
TextLayer *h_text_layer,*m_text_layer;
char Hbuffer[] = "00";
char Mbuffer[] = "00";

static Disc disc_array[NUM_DISCS];
static GRect window_frame;

//------------------------------------------------------------------------------------
static double disc_calc_mass(Disc *disc) {
	return MATH_PI * disc->radius * disc->radius * DISC_DENSITY;
}
//------------------------------------------------------------------------------------
static void disc_init(Disc *disc) {
	static double next_radius = 25;

	GRect frame = window_frame;
	disc->pos.x = frame.size.w/2;
	disc->pos.y = frame.size.h/2;
	disc->vel.x = 0;
	disc->vel.y = 0;
	disc->radius = next_radius;
	disc->mass = disc_calc_mass(disc);
	disc->color = GColorFromRGB(rand() % 255, rand() % 255, rand() % 255);
	next_radius -= 3;
}
//------------------------------------------------------------------------------------
static void disc_apply_force(Disc *disc, Vec2d force) {
	disc->vel.x += force.x / disc->mass;
	disc->vel.y += force.y / disc->mass;
}
//------------------------------------------------------------------------------------
static void disc_apply_accel(Disc *disc, AccelData accel) {
	disc_apply_force(disc, (Vec2d) {
	.x = accel.x * ACCEL_RATIO,
	.y = -accel.y * ACCEL_RATIO
	});
}
//------------------------------------------------------------------------------------
static void disc_update(Disc *disc) {
	double e = 0.3;

	if ((disc->pos.x - disc->radius < 0 && disc->vel.x < 0)
		|| (disc->pos.x + disc->radius > window_frame.size.w && disc->vel.x > 0)) {
		disc->vel.x = -disc->vel.x * e;
	}

	if ((disc->pos.y - disc->radius < 0 && disc->vel.y < 0)
		|| (disc->pos.y + disc->radius > window_frame.size.h && disc->vel.y > 0)) {
		disc->vel.y = -disc->vel.y * e;
	}

	disc->pos.x += disc->vel.x;
	disc->pos.y += disc->vel.y;
}
//------------------------------------------------------------------------------------
static void disc_draw(GContext *ctx, Disc *disc) {
	graphics_context_set_stroke_width(ctx, 4);
//	graphics_context_set_fill_color(ctx, disc->color);
	graphics_context_set_stroke_color(ctx, disc->color);
//	graphics_fill_circle(ctx, GPoint(disc->pos.x, disc->pos.y), disc->radius);
	graphics_draw_circle(ctx, GPoint(disc->pos.x, disc->pos.y), disc->radius);
}
//------------------------------------------------------------------------------------
static void disc_layer_update_callback(Layer *me, GContext *ctx) {
	for (int i = 0; i < NUM_DISCS; i++) {
		disc_draw(ctx, &disc_array[i]);
	}
}
//------------------------------------------------------------------------------------
static void timer_callback(void *data) {
	AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
	accel_service_peek(&accel);

	for (int i = 0; i < NUM_DISCS; i++) {
		Disc *disc = &disc_array[i];
		disc_apply_accel(disc, accel);
		disc_update(disc);
	}

	layer_mark_dirty(s_disc_layer);
	app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}
//------------------------------------------------------------------------------------
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
	layer_mark_dirty(display_layer);

	strftime(Hbuffer, sizeof("00"), "%H", tick_time);
	text_layer_set_text(h_text_layer, Hbuffer);
	strftime(Mbuffer, sizeof("00"), "%M", tick_time);
	text_layer_set_text(m_text_layer, Mbuffer);
}
//------------------------------------------------------------------------------------
static void main_window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect frame = window_frame = layer_get_frame(window_layer);

	s_disc_layer = layer_create(frame);
	layer_set_update_proc(s_disc_layer, disc_layer_update_callback);
	layer_add_child(window_layer, s_disc_layer);

	for (int i = 0; i < NUM_DISCS; i++) {
		disc_init(&disc_array[i]);
	}

	GRect bounds = layer_get_bounds(window_layer);
	

	
	//Time layer
	h_text_layer = text_layer_create(GRect(20, 14, 90, 50));
	text_layer_set_background_color(h_text_layer, GColorClear);
	text_layer_set_text_color(h_text_layer, GColorWhite);
	text_layer_set_text_alignment(h_text_layer, GTextAlignmentCenter);
	text_layer_set_font(h_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	layer_add_child(window_get_root_layer(window), (Layer*) h_text_layer);	

	m_text_layer = text_layer_create(GRect(40, 84, 90, 50));
	text_layer_set_background_color(m_text_layer, GColorClear);
	text_layer_set_text_color(m_text_layer, GColorWhite);
	text_layer_set_text_alignment(m_text_layer, GTextAlignmentCenter);
	text_layer_set_font(m_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_BOLD_SUBSET_49));
	layer_add_child(window_get_root_layer(window), (Layer*) m_text_layer);	
	
	display_layer = layer_create(bounds);
	layer_add_child(window_layer, display_layer);
	tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
	layer_mark_dirty(display_layer);

	
	
	
}
//------------------------------------------------------------------------------------
static void main_window_unload(Window *window) {
	layer_destroy(s_disc_layer);
	text_layer_destroy(h_text_layer);
	text_layer_destroy(m_text_layer);
}
//------------------------------------------------------------------------------------
static void init(void) {
	s_main_window = window_create();
	window_set_background_color(s_main_window, GColorBlack);
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	window_stack_push(s_main_window, true);

	accel_data_service_subscribe(0, NULL);
	app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}
//------------------------------------------------------------------------------------
static void deinit(void) {
	tick_timer_service_unsubscribe();
	accel_data_service_unsubscribe();
	window_destroy(s_main_window);
}
//------------------------------------------------------------------------------------
int main(void) {
	init();
	app_event_loop();
	deinit();
}
