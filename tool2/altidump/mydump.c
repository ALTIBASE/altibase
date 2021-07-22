/** 
 *  Copyright (c) 1999~2017, Altibase Corp. and/or its affiliates. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License, version 3,
 *  as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <bfd.h>

void print_symbol(bfd *file, symbol_info *info);

static char *target = NULL;

static int print_width = 8;

bfd *file;

#define FILENAME filename

void
bfd_nonfatal (string)
    CONST char *string;
{
    CONST char *errmsg = bfd_errmsg (bfd_get_error ());

    if (string)
        fprintf (stderr, "%s: %s: %s\n", "sample", string, errmsg);
    else
        fprintf (stderr, "%s: %s\n", "sample", errmsg);
}

void
bfd_fatal (string)
    CONST char *string;
{
    bfd_nonfatal (string);
    exit (1);
}

char *filename;

#define valueof(x) ((x)->section->vma + (x)->value)

asymbol *sort_x;
asymbol *sort_y;

int compare(void *sym1, void *sym2)
{
    asymbol    *asym1, *asym2;
    symbol_info syminfo;
    
    asym1 = bfd_minisymbol_to_symbol (file, 0, sym1, sort_x);
    if (asym1 == NULL)
        bfd_fatal (bfd_get_filename (file));
    
    asym2 = bfd_minisymbol_to_symbol (file, 0, sym2, sort_y);
    if (asym2 == NULL)
        bfd_fatal (bfd_get_filename (file));
    
    if (valueof (asym1) != valueof (asym2))
        return valueof (asym1) < valueof (asym2) ? -1 : 1;
    
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("mydump filename \n");
        exit(0);
    }
    filename = argv[1];

    
    bfd_init();
    set_default_bfd_target();
    {
        char **matching;
        
        file = bfd_openr (FILENAME, target);
        if (file == NULL)
        {
            bfd_nonfatal (FILENAME);
            return -1;
        }
        
/*          sort_x = bfd_make_empty_symbol (file); */
/*          sort_y = bfd_make_empty_symbol (file); */
        
/*        
          if (bfd_check_format (file, bfd_archive))
          {
          printf("this is archive..\n");
          exit(1);
          }
          */
        if (bfd_check_format_matches (file, bfd_object, &matching))
        {
            // (*format->print_object_filename) (filename);
            // display_rel_file (file, NULL);
        }

        if ( (bfd_get_file_flags (file) & HAS_SYMS) == 0) /* no symbol */
        {
            printf("%s: no symbols", bfd_get_filename (file));
            return 0;
        }

        {/* read symbol */
            int  dynamic = 0;
            long symcount;
            PTR minisyms;
            unsigned int size;
            char buf[30];

            /* get count  of symbol */
            symcount = bfd_read_minisymbols (file, dynamic, &minisyms, &size);
            if (symcount < 0)
                bfd_fatal (bfd_get_filename (file));

            if (symcount == 0)
            {
                printf("%s: no symbols", bfd_get_filename (file));
                return;
            }
            /* sorting symbol */
            
            qsort((void *)minisyms, symcount, size, compare);
            
            /* mode 에 따른 출력 길이 32/64 bit => 8/16 byte */
            bfd_sprintf_vma (file, buf, (bfd_vma) -1);
            print_width = strlen (buf);
            {
                /* print symbol */
                
                asymbol *store;
                bfd_byte *from, *fromend;
                
                store = bfd_make_empty_symbol (file);
                if (store == NULL)
                    bfd_fatal (bfd_get_filename (file));
                
                from = (bfd_byte *) minisyms;
                fromend = from + symcount * size;
                
                for (; from < fromend; from += size)
                {
                    asymbol *sym;
                    symbol_info syminfo;
                    
                    sym = bfd_minisymbol_to_symbol (file, dynamic, from, store);
                    if (sym == NULL)
                        bfd_fatal (bfd_get_filename (file));

                    /* print */
                    
                    bfd_get_symbol_info (file, sym, &syminfo);
                    print_symbol(file, &syminfo);
                }
            }
        }
    }
    
   
    return 0;
}

void print_symbol(bfd *file, symbol_info *info)
{
    if (toupper(info->type) == 'T' && strlen(info->name) > 0)
    {
        /*
          printf("value : 0x%lx \n", info->value);
          printf("type  : 0x%x (%c)\n",  (long)info->type, info->type);
          printf("name  : %s\n\n", info->name);
          */
        printf("0x%08x(%d) : %s \n", info->value, info->value, info->name);
    }
    /*
      printf("     stab type : 0x%x (%c)\n",
      (long)info->stab_type,
      info->stab_type);
      printf("     stab desc : 0x%x \n",  (long)info->stab_desc);
      printf("     stab name : %s \n\n", info->stab_name);
      */
}

            
#if defined(NOT_DEF)
{
    /* filtering symbol */

    bfd_byte *from, *fromend, *to;
    asymbol *store;

    /* 빈 공간 하나 생성 */
    store = bfd_make_empty_symbol (file);
    if (store == NULL)
        bfd_fatal (bfd_get_filename (file));

                 /* 처음 및 끝 symbol 위치 연산 */
    from = (bfd_byte *) minisyms;
    fromend = from + (symcount * size);
    to = (bfd_byte *) minisyms;

    for (; from < fromend; from += size)
    {
        int keep = 0;
        asymbol *sym;

        /* from이 위치하고 있는 minisymbol로 부터
           실제 symbol의 정보를 구한다.(sym이 구조체 가리킴)
           */
                    
        sym = bfd_minisymbol_to_symbol (file,
                                        dynamic,
                                        (const PTR) from,
                                        store);
        if (sym == NULL)
            bfd_fatal (bfd_get_filename (file));
                    
        if (undefined_only)
            keep = bfd_is_und_section (sym->section);
        else if (external_only)
            keep = ((sym->flags & BSF_GLOBAL) != 0
                    || (sym->flags & BSF_WEAK) != 0
                    || bfd_is_und_section (sym->section)
                    || bfd_is_com_section (sym->section));
        else
            keep = 1;
                    
        if (keep
            && ! print_debug_syms
            && (sym->flags & BSF_DEBUGGING) != 0)
            keep = 0;
                    
        if (keep
            && sort_by_size
            && (bfd_is_abs_section (sym->section)
                || bfd_is_und_section (sym->section)))
            keep = 0;
                    
        if (keep
            && defined_only)
        {
            if (bfd_is_und_section (sym->section))
                keep = 0;
        }
                    
        if (keep)
        {
            memcpy (to, from, size);
            to += size;
        }
    }
}
#endif            
