/* GnomeCalculator - double precision simple calculator widget
 *
 * Author: George Lebl <jirka@5z.com>
 */

#include <config.h>

/* needed for values of M_E and M_PI under 'gcc -ansi -pedantic'
 * on GNU/Linux */
#ifndef _BSD_SOURCE
#  define _BSD_SOURCE 1
#endif
#include <sys/types.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "libgnome/libgnomeP.h"
#include "gnome-calculator.h"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "gnome-pixmap.h"

typedef void (*sighandler_t)(int);

#define FONT_WIDTH 20
#define FONT_HEIGHT 30
#define DISPLAY_LEN 13

typedef enum {
	CALCULATOR_NUMBER,
	CALCULATOR_FUNCTION,
	CALCULATOR_PARENTHESIS
} CalculatorActionType;

typedef gdouble (*MathFunction1) (gdouble);
typedef gdouble (*MathFunction2) (gdouble, gdouble);

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

static GdkPixmap * font_pixmap = NULL;
static int calculator_count = 0;

typedef struct _CalculatorButton CalculatorButton;
struct _CalculatorButton {
	char *name;
	GtkSignalFunc signal_func;
	gpointer data;
	gpointer invdata;
	gboolean convert_to_rad;
	guint keys[4]; /*key shortcuts, 0 terminated list,
			 make sure to increase the number if more is needed*/
};

typedef void (*GnomeCalcualtorResultChangedSignal) (GtkObject * object,
						    gdouble result,
						    gpointer data);


enum {
	RESULT_CHANGED_SIGNAL,
	LAST_SIGNAL
};

static gint gnome_calculator_signals[LAST_SIGNAL] = {0};

static void
gnome_calculator_marshal_signal_result_changed (GtkObject * object,
						GtkSignalFunc func,
						gpointer func_data,
						GtkArg * args)
{
	GnomeCalcualtorResultChangedSignal rfunc;

	rfunc = (GnomeCalcualtorResultChangedSignal) func;

	(*rfunc) (object, GTK_VALUE_DOUBLE (args[0]),
		  func_data);
}

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

	gnome_calculator_signals[RESULT_CHANGED_SIGNAL] =
		gtk_signal_new("result_changed",
			       GTK_RUN_LAST,
			       object_class->type,
			       GTK_SIGNAL_OFFSET(GnomeCalculatorClass,
			       			 result_changed),
			       gnome_calculator_marshal_signal_result_changed,
			       GTK_TYPE_NONE,
			       1,
			       GTK_TYPE_DOUBLE);

	class->result_changed = NULL;
	gtk_object_class_add_signals (object_class, gnome_calculator_signals, LAST_SIGNAL);

	calculator_count++;
}

#if 0 /*only used for debugging*/
static void
dump_stack(GnomeCalculator *gc)
{
	CalculatorStack *stack;
	GList *list;

	puts("STACK_DUMP start");
	for(list = gc->stack;list!=NULL;list = g_list_next(list)) {
		stack = list->data;
		if(stack == NULL)
			puts("NULL");
		else if(stack->type == CALCULATOR_PARENTHESIS)
			puts("(");
		else if(stack->type == CALCULATOR_NUMBER)
			printf("% .12lg\n", stack->d.number);
		else if(stack->type == CALCULATOR_FUNCTION)
			puts("FUNCTION");
		else
			puts("UNKNOWN");
	}
	puts("STACK_DUMP end");
}
#endif

static void
put_led_font(GnomeCalculator *gc)
{
	gchar *text = gc->result_string;
	GdkPixmap *p;
	GtkStyle *style;
	gint retval;
	gint i;
	gint x;

	if(!GTK_WIDGET_REALIZED(GTK_WIDGET(gc)))
		return;

	style = gtk_widget_get_style(gc->display);
	gtk_pixmap_get(GTK_PIXMAP(gc->display), &p, NULL);

	gdk_draw_rectangle(p, style->black_gc, 1, 0, 0, -1, -1);

        if (font_pixmap != NULL) {
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
do_error(GnomeCalculator *gc)
{
	gc->error = TRUE;
	strcpy(gc->result_string,"e");
	put_led_font(gc);
}

/*we handle sigfpe's so that we can find all the errors*/
static void
sigfpe_handler(int type)
{
	/*most likely, but we don't really care what the value is*/
	errno = ERANGE;
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

	errno = 0;

	{
		sighandler_t old = signal(SIGFPE,sigfpe_handler);
		stack->d.number = (*func)(first,second);
		signal(SIGFPE,old);
	}

	if(errno>0 ||
	   finite(stack->d.number)==0) {
		errno = 0;
		do_error(gc);
	}
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
		g_snprintf(format, sizeof(format), "%c .%dlg", '%', i);
		g_snprintf(buf, sizeof(buf), format, gc->result);
		if(strlen(buf)<=12)
			break;
	}
	strncpy(gc->result_string,buf,12);
	gc->result_string[12]='\0';

	if(GTK_WIDGET_REALIZED(gc))
		put_led_font(gc);

	gtk_signal_emit(GTK_OBJECT(gc),
			gnome_calculator_signals[RESULT_CHANGED_SIGNAL],
			gc->result);

}

static void
unselect_invert(GnomeCalculator *gc)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gc->invert_button),
				    FALSE);
	gc->invert=FALSE;
}

static gdouble
convert_num(gdouble num, GnomeCalculatorMode from, GnomeCalculatorMode to)
{
	if(to==from)
		return num;
	else if(from==GNOME_CALCULATOR_DEG)
		if(to==GNOME_CALCULATOR_RAD)
			return (num*M_PI)/180;
		else /*GRAD*/
			return (num*200)/180;
	else if(from==GNOME_CALCULATOR_RAD)
		if(to==GNOME_CALCULATOR_DEG)
			return (num*180)/M_PI;
		else /*GRAD*/
			return (num*200)/M_PI;
	else /*GRAD*/
		if(to==GNOME_CALCULATOR_DEG)
			return (num*180)/200;
		else /*RAD*/
			return (num*M_PI)/200;
}

static void
no_func(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	push_input(gc);
	reduce_stack(gc);
	if(gc->error) return;
	set_result(gc);

	unselect_invert(gc);
}

static void
simple_func(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	CalculatorStack *stack;
	CalculatorButton *but = data;
	MathFunction1 func = but->data;
	MathFunction1 invfunc = but->invdata;

	g_return_if_fail(func!=NULL);
	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	push_input(gc);

	if(!gc->stack) {
		unselect_invert(gc);
		return;
	}

	stack = gc->stack->data;
	if(stack->type!=CALCULATOR_NUMBER) {
		unselect_invert(gc);
		return;
	}

	/*only convert non inverting functions*/
	if(!gc->invert && but->convert_to_rad)
		stack->d.number = convert_num(stack->d.number,
					      gc->mode,
					      GNOME_CALCULATOR_RAD);

	errno = 0;
	{
		sighandler_t old = signal(SIGFPE,sigfpe_handler);
		if(!gc->invert || invfunc==NULL)
			stack->d.number = (*func)(stack->d.number);
		else
			stack->d.number = (*invfunc)(stack->d.number);
		signal(SIGFPE,old);
	}

	if(errno>0 ||
	   finite(stack->d.number)==0) {
		errno = 0;
		do_error(gc);
		return;
	}

	/*we are converting back from rad to mode*/
	if(gc->invert && but->convert_to_rad)
		stack->d.number = convert_num(stack->d.number,
					      GNOME_CALCULATOR_RAD,
					      gc->mode);

	set_result(gc);

	unselect_invert(gc);
}

static void
math_func(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	CalculatorStack *stack;
	CalculatorButton *but = data;
	MathFunction2 func = but->data;
	MathFunction2 invfunc = but->invdata;

	g_return_if_fail(func!=NULL);
	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	push_input(gc);

	if(!gc->stack) {
		unselect_invert(gc);
		return;
	}

	reduce_stack(gc);
	if(gc->error) return;
	set_result(gc);

	stack = gc->stack->data;
	if(stack->type!=CALCULATOR_NUMBER) {
		unselect_invert(gc);
		return;
	}
	
	stack = g_new(CalculatorStack,1);
	stack->type = CALCULATOR_FUNCTION;
	if(!gc->invert || invfunc==NULL)
		stack->d.func = func;
	else
		stack->d.func = invfunc;

	gc->stack = g_list_prepend(gc->stack,stack);

	unselect_invert(gc);
}

static void
reset_calc(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc;
	if(w)
		gc = gtk_object_get_user_data(GTK_OBJECT(w));
	else
		gc = data;

	g_return_if_fail(gc!=NULL);

	while(gc->stack)
		stack_pop(&gc->stack);

	gc->result = 0;
	strcpy(gc->result_string,"0");
	gc->memory = 0;
	gc->mode = GNOME_CALCULATOR_DEG;
	gc->invert = FALSE;
	gc->error = FALSE;

	gc->add_digit = TRUE;
	push_input(gc);
	set_result(gc);

	unselect_invert(gc);
}

static void
clear_calc(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	gc->result = 0;
	strcpy(gc->result_string,"0");
	gc->error = FALSE;
	gc->invert = FALSE;

	gc->add_digit = TRUE;
	push_input(gc);
	set_result(gc);

	unselect_invert(gc);
}


/**
 * gnome_calculator_clear
 * @gc: Pointer to GNOME calculator widget.
 * @reset: %FALSE to zero, %TRUE to reset calculator completely
 *
 * Description:
 * Resets the calculator back to zero.  If @reset is %TRUE, results
 * stored in memory and the calculator mode are cleared also.
 **/

void
gnome_calculator_clear(GnomeCalculator *gc, const gboolean reset)
{
	if (reset)
		reset_calc(NULL, gc);
	else
		clear_calc(NULL, gc);
}

static void
add_digit(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	CalculatorButton *but = data;
	gchar *digit = but->name;

	if(gc->error)
		return;

	/*if the string is set, used for EE*/
	if(but->data)
		digit = but->data;

	g_return_if_fail(gc!=NULL);
	g_return_if_fail(digit!=NULL);

	if(!gc->add_digit) {
		if(gc->stack) {
			CalculatorStack *stack=gc->stack->data;
			if(stack->type==CALCULATOR_NUMBER)
				stack_pop(&gc->stack);
		}
		gc->add_digit = TRUE;
		gc->result_string[0] = '\0';
	}

	unselect_invert(gc);

	if(digit[0]=='e') {
		if(strchr(gc->result_string,'e'))
			return;
		else if(strlen(gc->result_string)>9)
			return;
		else if(gc->result_string[0]=='\0')
			strcpy(gc->result_string," 1");
	} else if(digit[0]=='.') {
		if(strchr(gc->result_string,'.'))
			return;
		else if(strlen(gc->result_string)>10)
			return;
		else if(gc->result_string[0]=='\0')
			strcpy(gc->result_string," 0");
	} else { /*numeric*/
		if(strlen(gc->result_string)>11)
			return;
		else if (strcmp (gc->result_string, " 0") == 0 ||
			 gc->result_string[0]=='\0')
			strcpy(gc->result_string," ");
	}

	strcat(gc->result_string,digit);
	
	put_led_font(gc);

	sscanf(gc->result_string,"%lf",&gc->result);
}

static gdouble
c_neg(gdouble arg)
{
	return -arg;
}


static void
negate_val(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	char *p;

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	unselect_invert(gc);

	if(!gc->add_digit) {
		simple_func(w,data);
		return;
	}

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

	sscanf(gc->result_string,"%lf",&gc->result);

	put_led_font(gc);
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
		errno=ERANGE;
		return 0;
	}
	return arg1/arg2;
}

static gdouble
c_inv(gdouble arg1)
{
	if(arg1==0) {
		errno=ERANGE;
		return 0;
	}
	return 1/arg1;
}

static gdouble
c_pow2(gdouble arg1)
{
	return pow(arg1,2);
}

static gdouble
c_pow10(gdouble arg1)
{
	return pow(10,arg1);
}

static gdouble
c_powe(gdouble arg1)
{
	return pow(M_E,arg1);
}

static gdouble
c_fact(gdouble arg1)
{
	int i;
	gdouble r;
	if(arg1<0) {
		errno=ERANGE;
		return 0;
	}
	i = (int)(arg1+0.5);
	if((fabs(arg1-i))>1e-9) {
		errno=ERANGE;
		return 0;
	}
	for(r=1;i>0;i--)
		r*=i;

	return r;
}

static gdouble
set_result_to(GnomeCalculator *gc, gdouble result)
{
	gdouble old;

	if(gc->stack==NULL ||
	   ((CalculatorStack *)gc->stack->data)->type!=CALCULATOR_NUMBER) {
		gc->add_digit = TRUE;
		old = gc->result;
		gc->result = result;
		push_input(gc);
	} else {
		old = ((CalculatorStack *)gc->stack->data)->d.number;
		((CalculatorStack *)gc->stack->data)->d.number = result;
	}

	set_result(gc);

	return old;
}


/**
 * gnome_calculator_set
 * @gc: Pointer to GNOME calculator widget.
 * @result: New value of calculator buffer.
 *
 * Description:
 * Sets the value stored in the calculator's result buffer to the given
 * @result.
 **/

void
gnome_calculator_set(GnomeCalculator *gc, gdouble result)
{
	set_result_to(gc,result);
}

static void
store_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	push_input(gc);

	gc->memory = gc->result;

	put_led_font(gc);

	unselect_invert(gc);
}

static void
recall_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	set_result_to(gc,gc->memory);

	unselect_invert(gc);
}

static void
sum_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	push_input(gc);

	gc->memory += gc->result;

	put_led_font(gc);

	unselect_invert(gc);
}

static void
exchange_m(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	gc->memory = set_result_to(gc,gc->memory);

	unselect_invert(gc);
}

static void
invert_toggle(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	if(GTK_TOGGLE_BUTTON(w)->active)
		gc->invert=TRUE;
	else
		gc->invert=FALSE;
}

static void
drg_toggle(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	GtkWidget *label = GTK_BUTTON(w)->child;
	GnomeCalculatorMode oldmode;

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	oldmode = gc->mode;

	if(gc->mode==GNOME_CALCULATOR_DEG)
		gc->mode=GNOME_CALCULATOR_RAD;
	else if(gc->mode==GNOME_CALCULATOR_RAD)
		gc->mode=GNOME_CALCULATOR_GRAD;
	else
		gc->mode=GNOME_CALCULATOR_DEG;


	if(gc->mode==GNOME_CALCULATOR_DEG)
		gtk_label_set_text(GTK_LABEL(label),_("DEG"));
	else if(gc->mode==GNOME_CALCULATOR_RAD)
		gtk_label_set_text(GTK_LABEL(label),_("RAD"));
	else
		gtk_label_set_text(GTK_LABEL(label),_("GRAD"));

	/*convert if invert is on*/
	if(gc->invert) {
		CalculatorStack *stack;
		push_input(gc);
		stack = gc->stack->data;
		stack->d.number = convert_num(stack->d.number,
					      oldmode,gc->mode);
		set_result(gc);
	}

	unselect_invert(gc);
}

static void
set_pi(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	set_result_to(gc,M_PI);

	unselect_invert(gc);
}

static void
set_e(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	set_result_to(gc,M_E);

	unselect_invert(gc);
}

static void
add_parenth(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));

	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	if(gc->stack &&
	   ((CalculatorStack *)gc->stack->data)->type==CALCULATOR_NUMBER)
		((CalculatorStack *)gc->stack->data)->type =
			CALCULATOR_PARENTHESIS;
	else {
		CalculatorStack *stack;
		stack = g_new(CalculatorStack,1);
		stack->type = CALCULATOR_PARENTHESIS;
		gc->stack = g_list_prepend(gc->stack,stack);
	}

	unselect_invert(gc);
}

static void
sub_parenth(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = gtk_object_get_user_data(GTK_OBJECT(w));
	g_return_if_fail(gc!=NULL);

	if(gc->error)
		return;

	push_input(gc);
	reduce_stack(gc);
	if(gc->error) return;
	set_result(gc);

	if(gc->stack) {
		CalculatorStack *stack = gc->stack->data;
		if(stack->type==CALCULATOR_PARENTHESIS)
			stack_pop(&gc->stack);
		else if(g_list_next(gc->stack)) {
			stack = g_list_next(gc->stack)->data;
			if(stack->type==CALCULATOR_PARENTHESIS) {
				GList *n = g_list_next(gc->stack);
				gc->stack = g_list_remove_link(gc->stack,n);
				g_list_free_1(n);
			}
		}
	}

	unselect_invert(gc);
}

static void
gnome_calculator_realized(GtkWidget *w, gpointer data)
{
	GnomeCalculator *gc = GNOME_CALCULATOR(w);

	if(font_pixmap == NULL) {
                GdkPixbuf *pixbuf;
		gchar *file = gnome_pixmap_file("calculator-font.png");

		if(file == NULL) {
			g_warning("Can't find calculator-font.png");
		} else {
                        pixbuf = gdk_pixbuf_new_from_file("calculator-font.png");
                        if (pixbuf == NULL) {
                                g_warning("Can't load calculator-font.png");
                        } else {
                                gdk_pixbuf_render_pixmap_and_mask(pixbuf, &font_pixmap, NULL, 128);
                        }
                        g_free(file);
                }
	}

	gc->display =  gtk_pixmap_new(gdk_pixmap_new(GTK_WIDGET(gc)->window,
						     DISPLAY_LEN*FONT_WIDTH,
						     FONT_HEIGHT,
						     -1), NULL);

	gtk_box_pack_start(GTK_BOX(gc),gc->display,FALSE,FALSE,0);

	gtk_widget_show(gc->display);

	put_led_font(gc);
}

static const CalculatorButton buttons[8][5] = {
	{
		{N_("1/x"),(GtkSignalFunc)simple_func,c_inv,NULL,FALSE,{0}},
		{N_("x^2"),(GtkSignalFunc)simple_func,c_pow2,sqrt,FALSE,{0}},
		{N_("SQRT"),(GtkSignalFunc)simple_func,sqrt,c_pow2,FALSE,{0}},
		{N_("CE/C"),(GtkSignalFunc)clear_calc,NULL,NULL,FALSE,{GDK_Clear,0}},
		{N_("AC"),(GtkSignalFunc)reset_calc,NULL,NULL,FALSE,{0}}
	},{
		{NULL,NULL,NULL,NULL}, /*inverse button*/
		{N_("sin"),(GtkSignalFunc)simple_func,sin,asin,TRUE,{0}},
		{N_("cos"),(GtkSignalFunc)simple_func,cos,acos,TRUE,{0}},
		{N_("tan"),(GtkSignalFunc)simple_func,tan,atan,TRUE,{0}},
		{N_("DEG"),(GtkSignalFunc)drg_toggle,NULL,NULL,FALSE,{0}}
	},{
		{N_("e"),(GtkSignalFunc)set_e,NULL,NULL,FALSE,{0}},
		{N_("EE"),(GtkSignalFunc)add_digit,"e+",NULL,FALSE,{0}},
		{N_("log"),(GtkSignalFunc)simple_func,log10,c_pow10,FALSE,{0}},
		{N_("ln"),(GtkSignalFunc)simple_func,log,c_powe,FALSE,{0}},
		{N_("x^y"),(GtkSignalFunc)math_func,pow,NULL,FALSE,{'^',0}}
	},{
		{N_("PI"),(GtkSignalFunc)set_pi,NULL,NULL,FALSE,{0}},
		{N_("x!"),(GtkSignalFunc)simple_func,c_fact,NULL,FALSE,{'!',0}},
		{N_("("),(GtkSignalFunc)add_parenth,NULL,NULL,FALSE,{'(',0}},
		{N_(")"),(GtkSignalFunc)sub_parenth,NULL,NULL,FALSE,{')',0}},
		{N_("/"),(GtkSignalFunc)math_func,c_div,NULL,FALSE,{'/',GDK_KP_Divide,0}}
	},{
		{N_("STO"),(GtkSignalFunc)store_m,NULL,NULL,FALSE,{0}},
		{N_("7"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'7',GDK_KP_7,0}},
		{N_("8"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'8',GDK_KP_8,0}},
		{N_("9"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'9',GDK_KP_9,0}},
		{N_("*"),(GtkSignalFunc)math_func,c_mul,NULL,FALSE,{'*',GDK_KP_Multiply,0}}
	},{
		{N_("RCL"),(GtkSignalFunc)recall_m,NULL,NULL,FALSE,{0}},
		{N_("4"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'4',GDK_KP_4,0}},
		{N_("5"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'5',GDK_KP_5,0}},
		{N_("6"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'6',GDK_KP_6,0}},
		{N_("-"),(GtkSignalFunc)math_func,c_sub,NULL,FALSE,{'-',GDK_KP_Subtract,0}}
	},{
		{N_("SUM"),(GtkSignalFunc)sum_m,NULL,NULL,FALSE,{0}},
		{N_("1"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'1',GDK_KP_1,0}},
		{N_("2"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'2',GDK_KP_2,0}},
		{N_("3"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'3',GDK_KP_3,0}},
		{N_("+"),(GtkSignalFunc)math_func,c_add,NULL,FALSE,{'+',GDK_KP_Add,0}}
	},{
		{N_("EXC"),(GtkSignalFunc)exchange_m,NULL,NULL,FALSE,{0}},
		{N_("0"),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'0',GDK_KP_0,0}},
		{N_("."),(GtkSignalFunc)add_digit,NULL,NULL,FALSE,{'.',GDK_KP_Decimal,',',0}},
		{N_("+/-"),(GtkSignalFunc)negate_val,c_neg,NULL,FALSE,{0}},
		{N_("="),(GtkSignalFunc)no_func,NULL,NULL,FALSE,{'=',GDK_KP_Enter,GDK_Return,0}}
	}
};

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
	gc->invert = FALSE;
	gc->add_digit = FALSE;
	gc->accel = gtk_accel_group_new();

	gtk_signal_connect_after(GTK_OBJECT(gc),"realize",
				 GTK_SIGNAL_FUNC(gnome_calculator_realized),
				 NULL);

	table = gtk_table_new(8,5,TRUE);
	gtk_widget_show(table);

	gtk_box_pack_end(GTK_BOX(gc),table,TRUE,TRUE,0);

	for(x=0;x<5;x++) {
		for(y=0;y<8;y++) {
			const CalculatorButton *but = &buttons[y][x];
			if(but->name) {
				int i;
				w=gtk_button_new_with_label(_(but->name));
				gtk_signal_connect(GTK_OBJECT(w),"clicked",
						   but->signal_func,
						   (gpointer) but);

				for(i=0;but->keys[i]!=0;i++) {
				  gtk_widget_add_accelerator(w,
							     "clicked",
							     gc->accel,
							     but->keys[i],0,
							     GTK_ACCEL_VISIBLE);
				  gtk_widget_add_accelerator(w,
							     "clicked",
							     gc->accel,
							     but->keys[i],
							     GDK_SHIFT_MASK,
							     GTK_ACCEL_VISIBLE);
				  gtk_widget_add_accelerator(w,
							     "clicked",
							     gc->accel,
							     but->keys[i],
							     GDK_LOCK_MASK,
							     GTK_ACCEL_VISIBLE);
				}
				gtk_object_set_user_data(GTK_OBJECT(w),gc);
				gtk_widget_show(w);
				gtk_table_attach(GTK_TABLE(table),w,
						 x,x+1,y,y+1,
						 GTK_FILL | GTK_EXPAND |
						 	GTK_SHRINK,
						 0, 2, 2);
			}
		}
	}
	gc->invert_button=gtk_toggle_button_new_with_label(_("INV"));
	gtk_signal_connect(GTK_OBJECT(gc->invert_button),"toggled",
			   GTK_SIGNAL_FUNC(invert_toggle),
			   gc);
	gtk_object_set_user_data(GTK_OBJECT(gc->invert_button),gc);
	gtk_widget_show(gc->invert_button);
	gtk_table_attach_defaults(GTK_TABLE(table),gc->invert_button,0,1,1,2);
}


/**
 * gnome_calculator_new
 *
 * Description:
 * Creates a calculator widget, a window with all the common buttons and
 * functions found on a standard pocket calculator.
 *
 * Returns:
 * Pointer to newly-created calculator widget.
 **/

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

	if((--calculator_count)==0 &&
	   font_pixmap) {
		gdk_pixmap_unref(font_pixmap);
		font_pixmap = NULL;
	}

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * gnome_calculator_get_result
 * @gc: Pointer to GNOME calculator widget
 *
 * Returns:
 * Value currently stored in calculator buffer.
 **/

gdouble
gnome_calculator_get_result (GnomeCalculator *gc)
{
	g_return_val_if_fail (gc, 0.0);
	g_return_val_if_fail (GNOME_IS_CALCULATOR (gc), 0.0);

	return gc->result;
}
