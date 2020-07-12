/*
	files.h
*/
#ifndef _FILES_H
#define _FILES_H
#include <_stddef.h>


// save entry text field in currently opened file
bool save_file ();

// save entry text field in a file
bool save_file_as (const wchar* fileName);


// open a file and load into entry text field
bool open_file (const wchar* fileName);

// reload the currently opened file
bool reload_file ();


// start a new empty file newfile.txt
bool new_file ();

// notify user if main text field has been modified
bool check_save_changes ();

// if NULL then file does not exists
const wchar* get_file_name ();


bool save_all_objects ();

void take_camera_picture ();

#endif
