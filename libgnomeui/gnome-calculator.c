/* GnomeCalculator - double precision simple calculator widget
 *
 * Author: George Lebl <jirka@5z.com>
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include "libgnome/libgnome.h"
#include "gnome-calculator.h"

#include "pixmaps/calculator-font.xpm"
#define FONT_WIDTH 20
#define FONT_HEIGHT 30
#define DISPLAY_LEN 13

typedef enum {
	CALCULATOR_NUMBER,
	CALCULATOR_FUNCTION,
	CALCULATOR_PARENTHESIS,
} CalculatorActionType;

typedef gdouble (*MathFunction1) (gdouble arg1);
typedef gdouble (*MathFunction2) (gdouble arg1, gdouble arg2);

typedef struct _CalculatorStack CalculatorStack;
struct _CalculatorStack {
	CalculatorActionType type;
	union {
		MathFunction2 func;
		gdouble number;
	} d;
};


static void gnome_calculator_class_init	(GnomeCalculatorClass	*class);
static void gnome_calculator_init	(GnomeCalculator	*gc);
static void gnome_calculator_destroy	(GtkObject		*object);

static GtkVBoxClass *parent_class;

static GdkPixmap * font_pixmap=NULL;

guint
gnome_calculator_get_type (void)
{
	static guint calculator_type = 0;

	if (!calculator_type) {
		GtkTypeInfo calculator_info = {
			"GnomeCalculator",
			sizeof (GnomeCalculator),
			sizeof (GnomeCalculatorClass),
			(GtkClassInitFunc) gnome_calculator_class_init,
			(GtkObjectInitFunc) gnome_calculator_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		calculator_type = gtk_type_unique (gtk_vbox_get_type (),
						   &calculator_info);
	}

	return calculator_type;
}

static void
gnome_calculator_class_init (GnomeCalculatorClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;

	parent_class = gtk_type_class (gtk_vbox_get_type ());

	object_class->destroy = gnome_calculator_destroy;
}

static void
put_led_font(GnomeCalculator *gc)
{
	gchar *text = gc->result_string;
	GdkPixmap *p;
	GtkStyle *style;
	gint retval;
	gint i;
	gint x;

	style = gtk_widget_get_style(gc->display);
	gtk_pixmap_get(GTK_PIXMAP(gc->display), &p, NULL);

	gdk_draw_rectangle(p, style->black_gc, 1, 0, 0, -1, -1);

	if(gc->memory!=0) {
		gdk_draw_pixmap(p, style->white_gc, font_pixmap, 13*FONT_WIDTH,
				0, 0, 0, FONT_WIDTH, FONT_HEIGHT);
	}
	for(x=12,i=strlen(text)-1;i>=0;x--,i--) {
		if(text[i]>='0' && text[i]<='9')
			gdk_draw_pixmap(p, style->white_gc, font_pixmap,
					(text[i]-'0')*FONT_WIDTH, 0, 
					x*FONT_WIDTH, 0,
					FONT_WIDTH, FONT_HEIGHT);
		else if(text[i]=='.')
			gdk_draw_pixmap(p, style->white_gc, font_pixmap,
					10*FONT_WIDTH, 0, 
					x*FONT_WIDTH, 0,
					FONT_WIDTH, FONT_HEIGHT);
		else if(text[i]=='+')
			gdk_draw_pixmap(p, style->white_gc, font_pixmap,
					11*FONT_WIDTH, 0, 
					x*FONT_WIDTH, 0,
					FONT_WIDTH, FONT_HEIGHT);
		else if(text[i]=='-')
			gdk_draw_pixmap(p, style->white_gc, font_pixmap,
					12*FONT_WIDTH, 0, 
					x*FONT_WIDTH, 0,
					FONT_WIDTH, FONT_HEIGHT);
		else if(text[i]=='e')
			gdk_draw_pixmap(p, style->white_gc, font_pixmap,
					14*FONT_WIDTH, 0, 
					x*FONT_WIDTH, 0,
					FONT_WIDTH, FONT_HEIGHT);
	}
	gtk_signal_emit_by_name(GTK_OBJECT(gc->display), "draw", &retval);
}

static void
stack_pop(GList **stack)
{
	CalculatorStack *s;
	GList *p;

	g_return_if_fail(stack);

	if(*stack == NULL) {
		g_warning("Stack underflow!");
		return;
	}

	s = (*stack)->data;
	p = (*stack)->next;
	g_list_free_1(*stack);
	if(p) p->prev = NULL;
	*stack = p;
	g_free(s);
}

static void
reduce_stack(GnomeCalculator *gc)
{
	CalculatorStack *stack;
	GList *list;
	MathFunction2 func;
	gdouble first;
	gdouble second;

	if(!gc->stack)
		return;

	stack = gc->stack->data;
	if(stack->type!=CALCULATOR_NUMBER)
		return;

	second = stack->d.number;

	list=g_list_next(gc->stack);
	if(!list)
		return;

	stack = list->data;
	if(stack->type==CALCULATOR_PARENTHESIS)
		return;
	if(stack->type!=CALCULATOR_FUNCTION) {
		g_warning("Corrupt GnomeCalculator stack!");
		return;
	}
	func = stack->d.func;

	list=g_list_next(list);
	if(!list) {
		g_warning("Corrupt GnomeCalculator stack!");
		return;
	}

	stack = list->data;
	if(stack->type!=CALCULATOR_NUMBER) {
		g_warning("Corrupt GnomeCalculator stack!");
		return;
	}
	first = stack->d.number;

	stack_pop(&gc->stack);
	stack_pop(&gc->stack);

	stack->d.number = (*func)(first,second);
}

static void
push_input(GnomeCalculator *gc)
{
	if(gc->add_digit) {
		CalculatorStack *stack;
		stack = g_new(CalculatorStack,1);
		stack->type = CALCULATOR_NUMBER;
		stack->d.number = gc->result;
		gc->stack = g_list_prepend(gc->stack,stack);
		gc->add_digit = FALSE;
	}
}

static void
set_result(GnomeCalculator *gc)
{
	CalculatorStack *stack;
	gchar buf[80];
	gchar format[20];
	gint i;

	g_return_if_fail(gc!=NULL);

	if(!gc->stack)
		return;

	stack = gc->stack->data;
	if(stack->type!=CALCULATOR_NUMBER)
		return;

	gc->result = stack->d.number;

	for(i=12;i>0;i--) {
		g_snprintf(format,20,"%c .%dlg",'%',i);
		g_snprintf(buf,80,format,gc->result);
		if(strlen(buf)<=12)
			break;
	}
	strncpy(gc->result_string,buf,12);
	gc->result_string[12]='\0';

	put_led_font(gc);
}

static gint
no_func(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);

	push_input(gc);
	reduce_stack(gc);
	set_result(gc);

	return TRUE;
}


static gint
simple_func(GtkWidget *w, gpointer data)
{
	MathFunction1 func = data;
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	CalculatorStack *stack;

	g_return_val_if_fail(func!=NULL,TRUE);
	g_return_val_if_fail(gc!=NULL,TRUE);

	push_input(gc);

	if(!gc->stack)
		return TRUE;

	stack = gc->stack->data;
	if(stack->type!=CALCULATOR_NUMBER)
		return TRUE;

	reduce_stack(gc);

	stack->d.number = (*func)(stack->d.number);

	set_result(gc);

	return TRUE;
}

static gint
math_func(GtkWidget *w, gpointer data)
{
	MathFunction2 func = data;
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	CalculatorStack *stack;

	g_return_val_if_fail(func!=NULL,TRUE);
	g_return_val_if_fail(gc!=NULL,TRUE);

	push_input(gc);

	if(!gc->stack)
		return TRUE;

	stack = gc->stack->data;
	if(stack->type!=CALCULATOR_NUMBER)
		return TRUE;

	reduce_stack(gc);
	set_result(gc);
	
	stack = g_new(CalculatorStack,1);
	stack->type = CALCULATOR_FUNCTION;
	stack->d.func = func;

	gc->stack = g_list_prepend(gc->stack,stack);

	return TRUE;
}

static gint
reset_calc(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = data;

	g_return_val_if_fail(gc!=NULL,TRUE);

	while(gc->stack)
		stack_pop(&gc->stack);

	gc->result = 0;
	strcpy(gc->result_string,"0");
	gc->memory = 0;
	gc->mode = GNOME_CALCULATOR_DEG;
	gc->inverse = FALSE;

	gc->add_digit = TRUE;
	push_input(gc);
	set_result(gc);

	return TRUE;
}

static gint
clear_calc(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = data;

	g_return_val_if_fail(gc!=NULL,TRUE);

	while(gc->stack)
		stack_pop(&gc->stack);

	gc->result = 0;
	strcpy(gc->result_string,"0");
	gc->inverse = FALSE;

	gc->add_digit = TRUE;
	push_input(gc);
	set_result(gc);

	return TRUE;
}

static gint
add_digit(GtkWidget *w, gpointer data)
{
	gchar *digit = data;
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);
	g_return_val_if_fail(digit!=NULL,TRUE);

	if(!gc->add_digit) {
		if(gc->stack) {
			CalculatorStack *stack=gc->stack->data;
			if(stack->type==CALCULATOR_NUMBER)
				stack_pop(&gc->stack);
		}
		gc->add_digit = TRUE;
		gc->result_string[0] = '\0';
	}

	if(digit[0]=='e') {
		if(strchr(gc->result_string,'e'))
			return;
		if(strlen(gc->result_string)>9)
			return;
		if(gc->result_string[0]=='\0')
			strcpy(gc->result_string," 1");
	} else if(digit[0]=='.') {
		if(strchr(gc->result_string,'.'))
			return;
		if(strlen(gc->result_string)>10)
			return;
		if(gc->result_string[0]=='\0')
			strcpy(gc->result_string," 0");
	} else {
		if(strlen(gc->result_string)>11)
			return;
		if(gc->result_string[0]=='\0')
			strcpy(gc->result_string," ");
	}

	strcat(gc->result_string,digit);

	put_led_font(gc);

	sscanf(gc->result_string,"%lf",&gc->result);

	return TRUE;
}

static gdouble
c_neg(gdouble arg)
{
	return -arg;
}


static gint
negate_val(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = data;
	char *p;

	g_return_val_if_fail(gc!=NULL,TRUE);

	if(!gc->add_digit)
		return simple_func(w,c_neg);

	if((p=strchr(gc->result_string,'e'))!=NULL) {
		p++;
		if(*p=='-')
			*p='+';
		else
			*p='-';
	} else {
		if(gc->result_string[0]=='-')
			gc->result_string[0]=' ';
		else
			gc->result_string[0]='-';
	}

	put_led_font(gc);
}

static void
do_error(void)
{
	/*FIXME: do error stuff!*/
}

static gdouble
c_add(gdouble arg1, gdouble arg2)
{
	return arg1+arg2;
}

static gdouble
c_sub(gdouble arg1, gdouble arg2)
{
	return arg1-arg2;
}

static gdouble
c_mul(gdouble arg1, gdouble arg2)
{
	return arg1*arg2;
}

static gdouble
c_div(gdouble arg1, gdouble arg2)
{
	if(arg2==0) {
		do_error();
		return 0;
	}
	return arg1/arg2;
}

static gdouble
c_inv(gdouble arg1)
{
	if(arg1==0) {
		do_error();
		return 0;
	}
	return 1/arg1;
}

static gdouble
c_pow2(gdouble arg1)
{
	return pow(arg1,2);
}

static gint
store_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);

	push_input(gc);

	gc->memory = gc->result;

	put_led_font(gc);

	return TRUE;
}

static gint
recall_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);

	if(gc->stack==NULL ||
	   ((CalculatorStack *)gc->stack->data)->type!=CALCULATOR_NUMBER) {
		gc->add_digit = TRUE;
		gc->result = gc->memory;
		push_input(gc);
	} else
		((CalculatorStack *)gc->stack->data)->d.number = gc->memory;

	set_result(gc);

	return TRUE;
}

static gint
sum_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);

	push_input(gc);

	gc->memory += gc->result;

	put_led_font(gc);

	return TRUE;
}

static gint
exchange_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	gdouble tmp;

	g_return_val_if_fail(gc!=NULL,TRUE);

	if(gc->stack==NULL ||
	   ((CalculatorStack *)gc->stack->data)->type!=CALCULATOR_NUMBER) {
		gc->add_digit = TRUE;
		tmp = gc->result;
		gc->result = gc->memory;
		push_input(gc);
	} else {
		tmp = ((CalculatorStack *)gc->stack->data)->d.number;
		((CalculatorStack *)gc->stack->data)->d.number = gc->memory;
	}
	gc->memory = tmp;

	set_result(gc);

	return TRUE;
}

static gint
drg_toggle(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	GtkWidget *label = GTK_BUTTON(w)->child;

	g_return_val_if_fail(gc!=NULL,TRUE);

	if(gc->mode==GNOME_CALCULATOR_DEG)
		gc->mode=GNOME_CALCULATOR_RAD;
	else if(gc->mode==GNOME_CALCULATOR_RAD)
		gc->mode=GNOME_CALCULATOR_GRAD;
	else
		gc->mode=GNOME_CALCULATOR_DEG;


	if(gc->mode==GNOME_CALCULATOR_DEG)
		gtk_label_set(GTK_LABEL(label),"DEG");
	else if(gc->mode==GNOME_CALCULATOR_RAD)
		gtk_label_set(GTK_LABEL(label),"RAD");
	else
		gtk_label_set(GTK_LABEL(label),"GRAD");

	return TRUE;
}

static gint
set_pi(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);

	if(gc->stack==NULL ||
	   ((CalculatorStack *)gc->stack->data)->type!=CALCULATOR_NUMBER) {
		gc->add_digit = TRUE;
		gc->result = M_PI;
		push_input(gc);
	} else
		((CalculatorStack *)gc->stack->data)->d.number = M_PI;

	set_result(gc);

	return TRUE;
}

static gint
set_e(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_val_if_fail(gc!=NULL,TRUE);

	if(gc->stack==NULL ||
	   ((CalculatorStack *)gc->stack->data)->type!=CALCULATOR_NUMBER) {
		gc->add_digit = TRUE;
		gc->result = M_E;
		push_input(gc);
	} else
		((CalculatorStack *)gc->stack->data)->d.number = M_E;

	set_result(gc);

	return TRUE;
}

static gint
gnome_calculator_realized(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = GNOME_CALCULATOR(w);

	if(!font_pixmap) {
		GdkColor black;
		gdk_color_black(gtk_widget_get_colormap(GTK_WIDGET(gc)),
							&black);
		font_pixmap = gdk_pixmap_create_from_xpm_d(GTK_WIDGET(gc)->
								window,
							   NULL, &black,
							   calculator_font_xpm);
	}

	gc->display =  gtk_pixmap_new(gdk_pixmap_new(GTK_WIDGET(gc)->window,
						     DISPLAY_LEN*FONT_WIDTH,
						     FONT_HEIGHT,
						     -1), NULL);

	gtk_box_pack_start(GTK_BOX(gc),gc->display,FALSE,FALSE,0);

	gtk_widget_show(gc->display);

	gc->stack = NULL;
	reset_calc(NULL,gc);
}

static void
gnome_calculator_init (GnomeCalculator *gc)
{
	gint x,y;
	GtkWidget *table;
	GtkWidget *w;

	gc->display = NULL;
	gc->stack = NULL;
	gc->result = 0;
	strcpy(gc->result_string,"0");
	gc->memory = 0;
	gc->mode = GNOME_CALCULATOR_DEG;
	gc->inverse = FALSE;
	gc->add_digit = FALSE;

	gtk_signal_connect_after(GTK_OBJECT(gc),"realize",
				 GTK_SIGNAL_FUNC(gnome_calculator_realized),
				 NULL);

	table = gtk_table_new(8,5,TRUE);
	gtk_widget_show(table);

	gtk_box_pack_end(GTK_BOX(gc),table,TRUE,TRUE,0);

	w=gtk_button_new_with_label("1/x");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(simple_func),
			   c_inv);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,0,1);

	w=gtk_button_new_with_label("x^2");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(simple_func),
			   c_pow2);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,1,2,0,1);
	
	w=gtk_button_new_with_label("SQRT");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(simple_func),
			   sqrt);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,2,3,0,1);

	w=gtk_button_new_with_label("CE/C");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(clear_calc),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,3,4,0,1);

	w=gtk_button_new_with_label("AC");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(reset_calc),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,0,1);

	w=gtk_button_new_with_label("DEG");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(drg_toggle),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,1,2);

	w=gtk_button_new_with_label("e");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(set_e),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,2,3);

	w=gtk_button_new_with_label("EE");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("e+"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,1,2,2,3);

	w=gtk_button_new_with_label("log");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(simple_func),
			   log10);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,2,3,2,3);

	w=gtk_button_new_with_label("ln");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(simple_func),
			   log);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,3,4,2,3);

	w=gtk_button_new_with_label("x^y");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(math_func),
			   pow);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,2,3);

	w=gtk_button_new_with_label("PI");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(set_pi),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,3,4);

	w=gtk_button_new_with_label("/");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(math_func),
			   c_div);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,3,4);

	w=gtk_button_new_with_label("STO");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(store_m),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,4,5);

	w=gtk_button_new_with_label("7");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("7"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,1,2,4,5);

	w=gtk_button_new_with_label("8");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("8"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,2,3,4,5);

	w=gtk_button_new_with_label("9");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("9"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,3,4,4,5);

	w=gtk_button_new_with_label("*");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(math_func),
			   c_mul);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,4,5);

	w=gtk_button_new_with_label("RCL");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(recall_m),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,5,6);

	w=gtk_button_new_with_label("4");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("4"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,1,2,5,6);

	w=gtk_button_new_with_label("5");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("5"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,2,3,5,6);

	w=gtk_button_new_with_label("6");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("6"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,3,4,5,6);

	w=gtk_button_new_with_label("-");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(math_func),
			   c_sub);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,5,6);

	w=gtk_button_new_with_label("SUM");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(sum_m),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,6,7);

	w=gtk_button_new_with_label("1");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("1"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,1,2,6,7);

	w=gtk_button_new_with_label("2");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("2"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,2,3,6,7);

	w=gtk_button_new_with_label("3");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("3"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,3,4,6,7);

	w=gtk_button_new_with_label("+");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(math_func),
			   c_add);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,6,7);

	w=gtk_button_new_with_label("EXC");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(exchange_m),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,0,1,7,8);

	w=gtk_button_new_with_label("0");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("0"));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,1,2,7,8);

	w=gtk_button_new_with_label(".");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(add_digit),
			   g_strdup("."));
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,2,3,7,8);

	w=gtk_button_new_with_label("+/-");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(negate_val),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,3,4,7,8);

	w=gtk_button_new_with_label("=");
	gtk_signal_connect(GTK_OBJECT(w),"clicked",
			   GTK_SIGNAL_FUNC(no_func),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(w),gc);
	gtk_widget_show(w);
	gtk_table_attach_defaults(GTK_TABLE(table),w,4,5,7,8);
}

GtkWidget *
gnome_calculator_new (void)
{
	GnomeCalculator *gcalculator;

	gcalculator = gtk_type_new (gnome_calculator_get_type ());

	return GTK_WIDGET (gcalculator);
}

static void
gnome_calculator_destroy (GtkObject *object)
{
	GnomeCalculator *gc;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CALCULATOR (object));

	gc = GNOME_CALCULATOR (object);

	while(gc->stack)
		stack_pop(&gc->stack);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
