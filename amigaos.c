#include <string.h>
#include <sys/stat.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef NEWLIB
#include <dos.h>
#endif

#include "sh.h"

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/utility.h>

#define FUNC //printf("<%s>\n", __PRETTY_FUNCTION__);fflush(stdout);
#define FUNCX //printf("</%s>\n", __PRETTY_FUNCTION__);fflush(stdout);

#ifndef NEWLIB
# define __USE_RUNCOMMAND__
#endif

#ifndef NEWLIB
/* clib2 specific controls */
BOOL __open_locale = FALSE;
BOOL __expand_wildcard_args = FALSE;

void __execve_exit(int return_code)
{
	lastresult = return_code;
}

int __execve_environ_init(char * const envp[])
{
    if (envp == NULL)
        return 0;

    /* Set a loal var to indicate to any subsequent sh that it is not */
    /* The top level shell and so should only inherit local amigaos vars */
    SetVar("ABCSH_IMPORT_LOCAL","true",5,GVF_LOCAL_ONLY);

    while (*envp != NULL)
    {
        int len;
        char *var;
        char *val;

        if ((len = strlen(*envp)))
        {
            size_t size = len + 1;
            if ((var = malloc(size)))
            {
                memcpy(var, *envp, size);

                val = strchr(var,'=');
                if(val)
                {
                    *val++='\0';
                    if (*val)
                    {
                        SetVar(var,val,strlen(val)+1,GVF_LOCAL_ONLY);
                    }
                }
                free(var);
                var = NULL;
            }
        }
        envp++;
    }
    return 0;
}

#else
int lastresult;
#endif

/*used to stdin/out fds*/

int amigaos_isabspath(const char *path)
{FUNC;
        if (*path == '/')
        {
                /* Special treatment: absolute path for Unix compatibility */
                FUNCX;
                return 1;
        }

        while (*path)
        {
                if (*path == ':')
                {
                        FUNCX;
                        return 1;
                }
                if (*path == '/')
                {
                        FUNCX;
                        return 0;
                }
                path++;
        }

        FUNCX;
        return 0;
}

int amigaos_isrootedpath(const char *path)
{
        return amigaos_isabspath(path);
}

int amigaos_isrelpath(const char *path)
{
        return !amigaos_isabspath(path);
}

int amigaos_dupbbase(int fd, int base)
{
        int res;
        FUNC;
        res = fcntl(fd, F_DUPFD, base);
        FUNCX;

        return res;
}

int getppid(void)
{
        return 0;
}


unsigned int alarm(unsigned int seconds)
{
        fprintf(stderr,"alarm() not implemented\n");
        return 1;
}

static int pipenum = 0;

int pipe(int filedes[2])
{
        char pipe_name[1024];

#ifdef USE_TEMPFILES
#if defined(__amigaos4__)
        snprintf(pipe_name, sizeof(pipe_name), "/T/%x.%08x",
                 pipenum++,((struct Process*)FindTask(NULL))->pr_ProcessID);
#else
        snprintf(pipe_name, sizeof(pipe_name), "/T/%x.%08x",
                 pipenum++,GetUniqueID());
#endif
#else
#if defined(__amigaos4__)
        snprintf(pipe_name, sizeof(pipe_name), "/PIPE/%x%lu/32768/0",
                 pipenum++,((struct Process*)FindTask(NULL))->pr_ProcessID);
#else
        snprintf(pipe_name, sizeof(pipe_name), "/PIPE/%x%08x/4096/0",
                 pipenum++,GetUniqueID());
#endif
#endif

        filedes[1] = open(pipe_name, O_WRONLY|O_CREAT);
        filedes[0] = open(pipe_name, O_RDONLY);

    if (filedes[0] == -1 || filedes[1] == -1)
        {
                if (filedes[0] != -1)
                    close(filedes[0]);
                if (filedes[1] != -1)
                    close(filedes[1]);

                FUNCX;
                return -1;
        }

        FUNCX;
        return 0;
}

int fork(void)
{
        fprintf(stderr,"fork() not implemented\n");
        errno = ENOMEM;
        return -1;
}

int wait(int *status)
{
        fprintf(stderr,"wait() not implemented\n");
        errno = ECHILD;
        return -1;
}

#ifdef NEWLIB
void __translate_path(const char *in, char *out)
{
	int absolute = (in[0] == '/');
	int dot = 0;
	int slashign = 0;

	if (absolute) in++;

	while (*in)
	{
		if (slashign && in[0] == '/')
		{
			slashign = 0;
		}
		else if (absolute && in[0] == '/')
		{
			/* Absolute path, replace first / with : */
			*out++ = ':';
			absolute = 0;
		}
		else if (dot)
		{
			dot = 0;
			/* Previous char was a dot */
			if (in[0] == '.')
			{
				/* Current is also a dot, make it a parent in out */
				*out++ = '/';
			}
			else if (in[0] == '/')
			{
				/* Current dir, ignore the following slash */
				slashign = 1;
			}
			else
			{
				/* A "solitary" dot, output it */
				*out++ = '.';
			}
		}
		else
		{
			if (in[0] == '.')
			{
				dot = 1;
			}
			else
				*out ++ = *in;
		}

		in++;
	}

	*out = '\0';
}
#endif

char *convert_path_a2u(const char *filename)
{
#ifndef NEWLIB
    struct name_translation_info nti;

    if(!filename)
    {
        return 0;
    }

    __translate_amiga_to_unix_path_name(&filename,&nti);
#endif
    return strdup(filename);

}

char *convert_path_multi(const char *path)
{
#ifndef NEWLIB
        struct name_translation_info nti;
        bool have_colon = false;
        char *p = (char *)path;

        while(p<path + strlen(path))
        {
            if(*p++ == ':') have_colon=true;
        }
        if(have_colon)
        {
            __translate_amiga_to_unix_path_name(&path,&nti);
        }
#endif
        return strdup(path);
}

char *convert_path_u2a(const char *filename)
{
#ifndef NEWLIB
        struct name_translation_info nti;
#else
        char buffer[MAXPATHLEN];
#endif
        FUNC;

        if (!filename)
        {
            FUNCX;
            return 0;
        }

        if (strcmp(filename, "/dev/tty") == 0)
        {
            return  strdup( "CONSOLE:");;
            FUNCX;

        }

#ifndef NEWLIB
        __translate_unix_to_amiga_path_name(&filename,&nti);
#else
        __translate_path(filename,buffer);
#endif
        return strdup(filename);
}

#ifdef NEWLIB
static void
createvars(char * const* envp)
{
    if (envp == NULL)
        return;

    /* Set a loal var to indicate to any subsequent sh that it is not */
    /* The top level shell and so should only inherit local amigaos vars */
    SetVar("ABCSH_IMPORT_LOCAL","true",5,GVF_LOCAL_ONLY);

    while (*envp != NULL)
    {
        int len;
        char *var;
        char *val;

        if ((len = strlen(*envp)))
        {
            size_t size = len + 1;
            if ((var = malloc(size)))
            {
                memcpy(var, *envp, size);

                val = strchr(var,'=');
                if(val)
                {
                    *val++='\0';
                    if (*val)
                    {
                        SetVar(var,val,strlen(val)+1,GVF_LOCAL_ONLY);
                    }
                }
                free(var);
                var = NULL;
            }
        }
        envp++;
    }
}

static bool contains_whitespace(char *string)
{
    if(strchr(string,' ')) return true;
    if(strchr(string,'\t')) return true;
    if(strchr(string,'\n')) return true;
    if(strchr(string,0xA0)) return true;
    if(strchr(string,'"')) return true;
    return false;
}

static int no_of_escapes(char *string)
{
    int cnt = 0;
    char *p;
    for(p=string;p<string + strlen(string);p++)
    {
        if(*p=='"') cnt++;
        if(*p=='*') cnt++;
        if(*p=='\n') cnt++;
        if(*p=='\t') cnt++;
    }
    return cnt;
}

struct command_data
{
    STRPTR args;
    BPTR seglist;
    struct Task *parent;
};


int execve(const char *filename, char *const argv[], char *const envp[])
{
        FILE *fh;
        char buffer[1000];
        size_t size = 0;
        char **cur;
        char *interpreter = 0;
        char * interpreter_args = 0;
        char *full = 0;
        char *filename_conv = 0;
        char *interpreter_conv = 0;
        char *fname;
        struct Task *thisTask = FindTask(0);

        FUNC;

        /* Calculate the size of filename and all args, including spaces and quotes */
        size = 0;
        for (cur = (char **)argv+1; *cur; cur++)
        {
            size += strlen(*cur) + 1 + (contains_whitespace(*cur)?(2 + no_of_escapes(*cur)):0);
        }

        /* Check if it's a script file */
        fh = fopen(filename, "r");
        if (fh)
        {
                if (fgetc(fh) == '#' && fgetc(fh) == '!')
                {
                        char *p;
                        char *q;
                        fgets(buffer, sizeof(buffer)-1, fh);
                        p = buffer;
                        while (*p == ' ' || *p == '\t') p++;

                        int buffer_len = strlen(buffer);
                        if ( buffer_len > 0 ) {
                                if(buffer[buffer_len-1] == '\n') {
                                        buffer[buffer_len-1] = '\0';
                                }
                        }

                        if ((q = strchr(p,' ')))
                        {
                            *q++ = '\0';
                            if(*q != '\0')
                            {
                            interpreter_args = strdup(q);
                            }
                        }
                        else
                            interpreter_args = strdup("");

                        interpreter = strdup(p);
                        size += strlen(interpreter) + 1;
                        size += strlen(interpreter_args) +1;
                }

                fclose(fh);
        }
        else
        {
            /* We couldn't open this why not? */
            if(errno == ENOENT)
            {
                /* file didn't exist! */
                return -1;
            }
        }

        /* Allocate the command line */
        if (filename)
            filename_conv = convert_path_u2a(filename);

        if(filename_conv)
            size += strlen(filename_conv);

        size += 1 + 10;  /* +10 for safety */
        full = malloc(size);
        if (full)
        {
            if (interpreter)
            {
                interpreter_conv = convert_path_u2a(interpreter);
#if !defined(__USE_RUNCOMMAND__)
                sprintf(full, "%s %s %s ", interpreter_conv, interpreter_args,filename_conv);
#else
                sprintf(full, "%s %s ",interpreter_args, filename_conv);
#endif
                free(interpreter);
                interpreter = NULL;
                free(interpreter_args);
                interpreter_args = NULL;

                if(filename_conv) {
                    free(filename_conv);
                    filename_conv = NULL;
                }

                fname = strdup(interpreter_conv);

                if(interpreter_conv) {
                    free(interpreter_conv);
                    interpreter_conv = NULL;
                }
            }
            else
            {
#ifndef __USE_RUNCOMMAND__
                snprintf(full, size, "%s ", filename_conv);
#else
                snprintf(full, size, "%.*s", 0, "");
#endif
                fname = strdup(filename_conv);
                if(filename_conv) {
                    free(filename_conv);
                    filename_conv = NULL;
                }
            }

            for (cur = (char**)argv+1; *cur != 0; cur++)
            {
                if(contains_whitespace(*cur))
                {
                    int esc = no_of_escapes(*cur);
                    if(esc > 0)
                    {
                        char *buff=malloc(strlen(*cur) + 4 + esc);
                        char *p = *cur;
                        char *q = buff;

                        *q++ = '"';
                        while(*p != '\0')
                        {
                            if(*p == '\n'){ *q++ = '*'; *q++ = 'N';p++;continue;}
                            else if(*p == '"'){ *q++ = '*'; *q++ = '"';p++;continue;}
                            else if(*p == '*' ){ *q++ = '*';}
                            *q++ = *p++;
                        }
                        *q++ = '"';
                        *q++ = ' ';
                        *q='\0';
                        strncat(full, buff, size);
                        free(buff);
                        buff = NULL;
                    }
                    else
                    {
                        strncat(full, "\"", size);
                        strncat(full, *cur, size);
                        strncat(full, "\" ", size);
                    }
                }
                else
                {
                    strncat(full, *cur, size);
                    strncat(full, " ", size);
                }

            }
            strncat(full, "\n", size);

            if (envp)
                createvars(envp);

#ifndef __USE_RUNCOMMAND__
            lastresult = SystemTags(full,
                SYS_UserShell,true,
                NP_StackSize,  ((struct Process *)thisTask)->pr_StackSize,
                SYS_Input,((struct Process *)thisTask)->pr_CIS,
                SYS_Output,((struct Process *)thisTask)->pr_COS,
                SYS_Error,((struct Process *)thisTask)->pr_CES,
              TAG_DONE);
#else
            if (fname){

                BPTR seglist = LoadSeg(fname);
                if(seglist)
                {
                    /* check if we have an executable */
                    struct PseudoSegList *ps = NULL;
                    if(!GetSegListInfoTags( seglist, GSLI_Native, &ps, TAG_DONE))
                    {
                        if(!GetSegListInfoTags( seglist, GSLI_68KPS, &ps, TAG_DONE))
                        {
                            GetSegListInfoTags( seglist, GSLI_68KHUNK, &ps, TAG_DONE);
                        }
                    }

                    if( ps != NULL )
                    {
                        SetProgramName(fname);
                        lastresult=RunCommand(seglist,((struct Process*)thisTask)->pr_StackSize,
                                              full,strlen(full));
                        errno=0;
                    }
                    else
                    {
                        errno=ENOEXEC;
                    }
                    UnLoadSeg(seglist);
                    seglist = 0;
                }
                else
                {
                    errno=ENOEXEC;
                }
               free(fname);
               fname = NULL;
            }

#endif /* USE_RUNCOMMAND */

            free(full);
            full = NULL;
            FUNCX;
            if(errno == ENOEXEC)
            {
                return -1;
            } else {
                return lastresult;
            }
        }

        if(interpreter) {
            free(interpreter);
            interpreter = NULL;
        }

        if(filename_conv) {
            free(filename_conv);
            filename_conv = NULL;
        }

        errno = ENOMEM;

        FUNCX;
        return -1;
}
#endif /* newlib */

int pause(void)
{
        fprintf(stderr,"Pause not implemented\n");

        errno = EINTR;
        return -1;
}

struct userdata
{
        struct op *t;
        int flags;
        struct Task *parent;
};

LONG execute_child(STRPTR args, int len)
{
        struct op *t;
        int flags;
#if !defined(__amigaos4__)
        struct Task *parent;
#endif
        struct Task *this;
        struct globals globenv;

        this = FindTask(0);
        t = ((struct userdata *)this->tc_UserData)->t;
        flags = ((struct userdata*)this->tc_UserData)->flags;
#if !defined(__amigaos4__)
        parent = ((struct userdata*)this->tc_UserData)->parent;
#endif
        copyenv(&globenv);
        e->type=E_SUBSHELL;
        if(!(ksh_sigsetjmp(e->jbuf,0)))
        {
            execute(t, flags & (XEXEC | XERROK));
        }
        else
        {
            lastresult = exstat;
        }
        restoreenv(&globenv);
#if !defined(__amigaos4__)
        Forbid();
        Signal(parent, SIGBREAKF_CTRL_F);
#endif

        return 0;
}

int
exchild(struct op *t, int flags,
        int close_fd)   /* used if XPCLOSE or XCCLOSE */
{
/*current input output*/
        int i;
/*close conditions*/
        long amigafd[3];
        struct Process *proc = NULL;
        struct Task *thisTask = FindTask(0);
        struct userdata taskdata;

        char *name = NULL;

        FUNC;
        if(flags & XEXEC)
        {
            execute(t,flags&(XEXEC | XERROK));
        }
#if 0
        printf("flags = ");
        if (flags & XEXEC) printf("XEXEC ");
        if (flags & XFORK) printf("XFORK ");
        if (flags & XBGND) printf("XBGND ");
        if (flags & XPIPEI) printf("XPIPEI ");
        if (flags & XPIPEO) printf("XPIPEO ");
        if (flags & XXCOM) printf("XXCOM ");
        if (flags & XPCLOSE) printf("XPCLOSE ");
        if (flags & XCCLOSE) printf("XCCLOSE ");
        if (flags & XERROK) printf("XERROK ");
        if (flags & XCOPROC) printf("XCOPROC ");
        if (flags & XTIME) printf("XTIME ");
        printf("\n"); fflush(stdout);
#endif

        taskdata.t      = t;
        taskdata.flags  = flags & (XEXEC | XERROK);
        taskdata.parent = thisTask;
        for(i = 0; i < 3; i++)
        {
            __get_default_file(i, &amigafd[i]);
        }

        if(t->str) {
            name = strdup(t->str);
        }
        else {
            name = strdup("new sh process");
        }

        proc = CreateNewProcTags(
            NP_Entry,                execute_child,
            NP_Child,                true,
            NP_StackSize,            ((struct Process *)thisTask)->pr_StackSize,
            NP_Input,                amigafd[0],
            NP_Output,               amigafd[1],
            NP_CloseOutput,          false,
            NP_CloseInput,           false,
            NP_Error,                amigafd[2],
            NP_CloseError,           false,
            NP_Cli,                  true,
            NP_Name,                 name,
            NP_CommandName,          name,
#ifdef __amigaos4__
            NP_UserData,             (int)&taskdata,
            NP_NotifyOnDeathSigTask, thisTask,
#endif
            TAG_DONE);

        if ( proc != NULL ) {
#ifndef __amigaos4__
#warning this code has been included!
                proc->pr_Task.tc_UserData = &taskdata;
                Wait(SIGBREAKF_CTRL_F);
#else
                Wait(SIGF_CHILD);
#endif
        }

        free(name);
        name = NULL;

          if((i=close_fd) >=0 && (flags & XPCLOSE) )
          {
#ifdef USE_TEMPFILES
                BPTR f=0;
                UBYTE pname[256];

                if(flags & XPIPEI)
                {
                      __get_default_file(i,&f);
                      NameFromFH(f,pname,sizeof(pname)-1);
                }

                close(i);
                if(f)
                {
                    DeleteFile(pname);
                }
#else  /* using true pipes */

                if(flags & XPIPEI)
                {

                    char buffer[256];
                    int n,t;
                    t = 0;
                    while((n = read(i,buffer,sizeof(buffer)-1)) > 0) t +=n;
                }

                close(i);
#endif  /* USE_TEMPFILES */

           }

        FUNCX;
        return lastresult;
}

#if 0
/* AmiUpdate compatibility code - http://www.amiupdate.net */

/**********************************************************
**
** The following function saves the variable name passed in 
** 'varname' to the ENV(ARC) system so that the application 
** can become AmiUpdate aware.
**
**********************************************************/
void
SetAmiUpdateENVVariable(const char *varname)
{
	/* AmiUpdate support code */
	BPTR lock;
	APTR oldwin;

	/* obtain the lock to the home directory */
	if((lock = GetProgramDir()) != 0)
	{
		TEXT progpath[2048];
		TEXT varpath[1024] = "AppPaths";

		/*
		 * get a unique name for the lock,
		 * this call uses device names,
		 * as there can be multiple volumes
		 * with the same name on the system
		 */

		if(DevNameFromLock(lock, progpath, sizeof(progpath),
					DN_FULLPATH))
		{
			/* stop any "Insert volume..." type requesters */
			oldwin = SetProcWindow((APTR)-1);

			/*
			 * finally set the variable to the
			 * path the executable was run from
			 * don't forget to supply the variable
			 * name to suit your application
			 */

			AddPart(varpath, varname, 1024);
			SetVar(varpath, progpath, -1,
					GVF_GLOBAL_ONLY|GVF_SAVE_VAR);

			/* turn requesters back on */
			SetProcWindow(oldwin);
		}
	}
}
#endif /* 0 */
