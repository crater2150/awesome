/*  
 * util.c - useful functions
 *  
 * Copyright © 2007 Julien Danjou <julien@danjou.info>
 * Copyright © 2006 Pierre Habouzit <madcoder@debian.org>
 * 
 * This program is free software; you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License as published by 
 * the Free Software Foundation; either version 2 of the License, or 
 * (at your option) any later version. 
 * 
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 * GNU General Public License for more details. 
 * 
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. 
 * 
 */

#include <stdarg.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xinerama.h>  

#include "util.h"

void
die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    abort();
}

void
eprint(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

void
uicb_spawn(Display * disp,
           DC *drawcontext __attribute__ ((unused)),
           awesome_config * awesomeconf __attribute__ ((unused)),
           const char *arg)
{
    static char *shell = NULL;
    char *display = NULL;
    char *tmp, newdisplay[128];

    if(!shell && !(shell = getenv("SHELL")))
        shell = a_strdup("/bin/sh");
    if(!arg)
        return;
    
    if(!XineramaIsActive(disp) && (tmp = getenv("DISPLAY")))
    {
        display = a_strdup(tmp);
        if((tmp = strrchr(display, '.')))
            *tmp = '\0';
        snprintf(newdisplay, sizeof(newdisplay), "%s.%d", display, awesomeconf->screen);
        setenv("DISPLAY", newdisplay, 1);
    }


    /* The double-fork construct avoids zombie processes and keeps the code
     * clean from stupid signal handlers. */
    if(fork() == 0)
    {
        if(fork() == 0)
        {
            if(disp)
                close(ConnectionNumber(disp));
            setsid();
            execl(shell, shell, "-c", arg, (char *) NULL);
            fprintf(stderr, "awesome: execl '%s -c %s'", shell, arg);
            perror(" failed");
        }
        exit(EXIT_SUCCESS);
    }
    wait(0);
}

Bool
xgettextprop(Display *disp, Window w, Atom atom, char *text, ssize_t textlen)
{
    char **list = NULL;
    int n;

    XTextProperty name;

    if(!text || !textlen)
        return False;

    text[0] = '\0';
    XGetTextProperty(disp, w, &name, atom);
 
    if(!name.nitems)
        return False;
 
    if(name.encoding == XA_STRING)
        a_strncpy(text, textlen, (char *) name.value, textlen - 1);

    else if(XmbTextPropertyToTextList(disp, &name, &list, &n) >= Success && n > 0 && *list)
    {
        a_strncpy(text, textlen, *list, textlen - 1);
        XFreeStringList(list);
    }

    text[textlen - 1] = '\0';
    XFree(name.value);

    return True;
}

double
compute_new_value_from_arg(const char *arg, double current_value)
{
    double delta;

    if(arg && sscanf(arg, "%lf", &delta) == 1)
    {
        if(arg[0] == '+' || arg[0] == '-')
            current_value += delta;
        else
            current_value = delta;
    }

    return current_value;
}

/** \brief safe limited strcpy.
 *
 * Copies at most min(<tt>n-1</tt>, \c l) characters from \c src into \c dst,
 * always adding a final \c \\0 in \c dst.
 *
 * \param[in]  dst      destination buffer.
 * \param[in]  n        size of the buffer. Negative sizes are allowed.
 * \param[in]  src      source string.
 * \param[in]  l        maximum number of chars to copy.
 *
 * \return minimum of  \c src \e length and \c l.
*/
ssize_t a_strncpy(char *dst, ssize_t n, const char *src, ssize_t l)
{
    ssize_t len = MIN(a_strlen(src), l);

    if (n > 0)
    {
        ssize_t dlen = MIN(n - 1, len);
        memcpy(dst, src, dlen);
        dst[dlen] = '\0';
    }

    return len; 
} 

/** \brief safe strcpy.
 *
 * Copies at most <tt>n-1</tt> characters from \c src into \c dst, always
 * adding a final \c \\0 in \c dst.
 *
 * \param[in]  dst      destination buffer.
 * \param[in]  n        size of the buffer. Negative sizes are allowed.
 * \param[in]  src      source string.
 * \return \c src \e length. If this value is \>= \c n then the copy was
 *         truncated.
 */
ssize_t a_strcpy(char *dst, ssize_t n, const char *src)
{
    ssize_t len = a_strlen(src);

    if (n > 0)
    {
        ssize_t dlen = MIN(n - 1, len);
        memcpy(dst, src, dlen);
        dst[dlen] = '\0';
    }

    return len;
}
