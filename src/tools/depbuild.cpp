/*
Copyright (C) 2005-2006 Nach, grinvader ( http://www.zsnes.com )

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
This is part of a toolkit used to assist in ZSNES development

This program generates dependacies for all C/C++/Assembly files
*/

#include <iostream>
#include <string>
#include <cstdio>
using namespace std;

#include "fileutil.h"
#include "strutil.h"

string cc;
string nasm;
string cflags;
string nflags;

void fix_line(string &line, const char *filename)
{
  if (line.find(":") != string::npos) //If this is the first line outputed for this file
  {
    string fname(filename);
    size_t last_slash = fname.find_last_of("/");
    if (last_slash != string::npos) //If it's in a subdirectory, add directory path to the object filename
    {
      line.replace(0, 3, fname, 0, last_slash+1);
    }
    else //Otherwise just remove the leading spaces
    {
      line.erase(0, 3);
    }
  }
  else //Sequal lines need to make sure previous line ends with a \ and then go to next line
  {
    cout << " \\\n";
  }
}

//This function is so crazy because GCC doesn't put in proper directories, and adds system headers
void dependancy_calculate_c(const char *filename)
{
  string command = cc + " " + cflags + " -M " + filename;
  FILE *fp = popen(command.c_str(), "r");
  if (fp)
  {
    char line[256];
    string processed_line("  ");
    bool line_read = false;
    while (fgets(line, sizeof(line), fp)) //Process all lines of output
    {
       line_read = true;
       vector<string> tokens;
       Tokenize(string(line), tokens, " \t\n\\"); //Break apart into each dependancy
       for (vector<string>::iterator i = tokens.begin(); i != tokens.end(); i++)
       {
         if ((*i)[0] != '/') //If dependancy isn't a system header (all system headers would begin with /)
         {
           //This if has to be before the dependacy is added onto the processed line string
           if (processed_line.length() > 50) //Let's wrap every time we go over 50 characters
           {
             fix_line(processed_line, filename);
             cout << processed_line;
             processed_line = "  ";
           }
           string dependancy = *i;
           //Now check if there is a needless dir/../
           size_t first_slash = dependancy.find_first_of("/");
           if ((first_slash != string::npos) && dependancy.compare(0, 2, "..") && !dependancy.compare(first_slash, 4, "/../"))
           {
             dependancy.erase(0, first_slash+strlen("/../"));
           }
           processed_line += " " + dependancy; //Add dependacy to current line. Output for overflow (wrapping) should be done before this
         }
       }
    }
    if (line_read) //Only output if there was dependancy data
    {
      fix_line(processed_line, filename);
      cout << processed_line << "\n";
    }
    pclose(fp);
  }
  else
  {
    cerr << "Failed on: " << filename << "\n";
  }
}

void dependancy_calculate_asm(const char *filename)
{
  string command = nasm + " " + nflags + " -M " + filename;
  system(command.c_str());
}

void dependancy_calculate(const char *filename, struct stat& stat_buffer)
{
  if (extension_match(filename, ".asm"))
  {
    dependancy_calculate_asm(filename);
  }
  else if (extension_match(filename, ".c") || extension_match(filename, ".cpp"))
  {
    dependancy_calculate_c(filename);
  }
}


int main(size_t argc, const char *const *const argv)
{
  if (argc < 5)
  {
    cout << "Usage: depbuild CC CFLAGS NASM NFLAGS\n"
         << "\n"
         << "Make sure to properly quote (and possibly escape) the 4 parameters being passed\n"
         << endl;
  }
  else
  {
    cc     = argv[1];
    cflags = argv[2];
    nasm   = argv[3];
    nflags = argv[4];

    if (argc == 5)
    {
      parse_dir(".", dependancy_calculate);
    }
    else
    {
      struct stat unused;
      for (size_t i = 5; i < argc; i++)
      {
        dependancy_calculate(argv[i], unused);
      }
    }
  }
  return(0);
}
