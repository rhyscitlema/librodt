/*
    files.h
*/
#ifndef _FILES_H
#define _FILES_H
#include <_stddef.h>


extern bool save_file ();           // save entry text field in currently opened file

extern bool save_file_as (const wchar* fileName); // save entry text field in a file


extern bool open_file (const wchar* fileName); // open a file and load into entry text field

extern bool reload_file ();         // reload the currently opened file


extern bool new_file ();            // start a new, untitled and empty file entry

extern bool check_save_changes ();  // notify user if entry text field has been modified


extern bool file_exists_get ();

extern const wchar* file_name_get ();


extern bool save_all_objects ();

extern void take_camera_picture ();

#endif
