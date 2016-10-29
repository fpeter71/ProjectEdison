#include "log.h"

/* Konstruktor
 *  arg1: napl�f�jl el�r�si �tja �s neve
 *  arg2: napl�z�si szint
 *  arg3: napl�z�s l�tsz�djon az alap�rtlemezett kimeneten?
 */
etm::Log::Log(char *fname, unsigned int level, bool terminal)
{
    _fp = NULL;      // f�jlmutat� be�ll�t�sa NULL �rt�kre ,hibakeres�s miatt

    _err_num = LOG_SUCCESS;

    /* napl�f�jl megnyit�sa, csak �r�sra */
    if((_fp = fopen(fname, "a")) == NULL){
    	_err_num = LOG_EOPEN;
        _is_open = false;    // nem siker�lt, ezt elt�roljuk a v�ltoz�ban
    } else
        _is_open = true;     // siker�lt, �gy ezt t�roljuk el

    /* tagv�ltoz�k be�ll�t�sa */
    _log_level = level;
    _show_terminal = terminal;
    _writing_is_happening = false;
}

/* �res kosntruktor, h�thakell
 */
etm::Log::Log(void){
	 _fp = NULL;
	 _err_num = LOG_SUCCESS;
	 _log_level = LOG_NONE;
	 _show_terminal = false;
	 _writing_is_happening = false;
}

/* Destruktor
 */
etm::Log::~Log(void)
{
    /* ha a napl�f�jl meg van nyitva */
    if(_is_open || _fp != NULL){
        fclose(_fp); // bez�rjuk
    }
}

/* Egy napl�bejegyz�s be�r�sa
 *  arg1: napl�bejegyz�s szintje
 *  arg2: napl�bejegyz�s sz�vege
 */
void etm::Log::write(char level, char *msg)
{
    /* ha a be�ll�tott szint megegyezik az �zenet szintj�vel �s a f�jl nyitva van, akkor napl�zunk */
    if(level & _log_level && _is_open){
    	 _err_num = LOG_SUCCESS;
        time_t t = time(NULL);          // aktu�lis id� lek�rdez�se
        struct tm tm = *localtime(&t);  // id� strukt�r�ba helyez�se

        char lvl[10];   // �zenet szintj�t jelz� karakterl�nc l�terhoz�sa

        /* �zenet szintj�t jelz� karakterl�nc �rt�k�nek be�ll�t�sa */
        switch(level){
        	case LOG_ERROR :
        		strcpy(lvl, "ERROR");
        		break;
        	case LOG_WARNING :
        		strcpy(lvl, "WARNING");
        		break;
        	case LOG_INFO :
        		strcpy(lvl, "INFO");
        		break;
        	case LOG_DISPLAY :
        		strcpy(lvl, "DISPLAY");
        		break;
        	default:
        		strcpy(lvl, "DAFUQ");
        }

        char log_msg[MSG_SIZE]; // napl�bejegyz�s lefoglal�sa

        /* napl�bejegyz�s felt�lt�se adatokkal */
        sprintf(log_msg, "%04d-%02d-%02dT%02d:%02d:%02d [%s] %s\n", tm.tm_year + 1900, tm.tm_mon + 1,
                 tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
                 lvl, msg);

        /* szinkroniz�l�s miatt, v�rjuk am�g esetleg m�s �r a f�jlba */
        while(_writing_is_happening);

        _writing_is_happening = true;
        /* napl�bejegyz�s f�jlba �r�sa*/
        if(fprintf(_fp, "%s", log_msg) != (int32_t)strlen(msg)){
        	_err_num = LOG_EWRITE;
        }
        _writing_is_happening = false;

        /* ha be van �ll�tva hogy a kimeneten is megjelensen a bejegyz�s */
        if(_show_terminal){
            printf("%s", log_msg);  // akkor megjelenik
        }
    }
}

/* Egy napl�bejegyz�s form�zott be�r�sa
 *  arg1: napl�bejegyz�s szintje
 *  arg2: napl�bejegyz�s sz�vege
 *  argn: form�tum param�terek
 */
void etm::Log::writef(char level, char *fmt, ...)
{
    char log_msg[MSG_SIZE];         // napl�bejegyz�s sz�vege

    va_list arglist;                // argumentumlista l�trehoz�sa

    va_start(arglist, fmt);         // argumentumlista �rt�keinek beolvas�sa a sz�veg alapj�n
    vsprintf(log_msg, fmt, arglist);    // argumentumlista behelyettes�t�se �s a bufferbe helyez�se
    va_end(arglist);                // lista lez�r�sa

    write(level, log_msg);          // a m�r sz�pen m�k�d� fv. megh�v�sa, �gy m�r tudja kezelni
}

/* Napl�f�jl meg van e nyitva?
 */
bool etm::Log::is_open(void) const
{
    return _is_open;     // ebb�l kider�l
}

/* Mi volt a hiba?
 */
int etm::Log::get_err_num(void) const
{
	return _err_num;	// ez
}
