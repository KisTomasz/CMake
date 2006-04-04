/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "cmForEachCommand.h"

bool cmForEachFunctionBlocker::
IsFunctionBlocked(const cmListFileFunction& lff, cmMakefile &mf) 
{
  // Prevent recusion and don't let this blobker block its own
  // commands.
  if (this->Executing)
    {
    return false;
    }
  
  // at end of for each execute recorded commands
  if (cmSystemTools::LowerCase(lff.Name) == "endforeach")
    {
    std::vector<std::string> expandedArguments;
    mf.ExpandArguments(lff.Arguments, expandedArguments);
    if(!expandedArguments.empty() && (expandedArguments[0] == this->Args[0]))
      {
      // store the old value
      std::string oldDef;
      if (mf.GetDefinition(this->Args[0].c_str()))
        {
        oldDef = mf.GetDefinition(this->Args[0].c_str());
        }
      this->Executing = true;
      std::vector<std::string>::const_iterator j = this->Args.begin();
      ++j;

      std::string tmps;
      cmListFileArgument arg;
      for( ; j != this->Args.end(); ++j)
        {   
        // set the variable to the loop value
        mf.AddDefinition(this->Args[0].c_str(),j->c_str());
        // Invoke all the functions that were collected in the block.
        for(unsigned int c = 0; c < this->Functions.size(); ++c)
          {
          mf.ExecuteCommand(this->Functions[c]);
          }
        }
      // restore the variable to its prior value
      mf.AddDefinition(this->Args[0].c_str(),oldDef.c_str());
      mf.RemoveFunctionBlocker(lff);
      return true;
      }
    }

  // record the command
  this->Functions.push_back(lff);
  
  // always return true
  return true;
}

bool cmForEachFunctionBlocker::
ShouldRemove(const cmListFileFunction& lff, cmMakefile& mf)
{
  if(cmSystemTools::LowerCase(lff.Name) == "endforeach")
    {
    std::vector<std::string> expandedArguments;
    mf.ExpandArguments(lff.Arguments, expandedArguments);
    if(!expandedArguments.empty() && (expandedArguments[0] == this->Args[0]))
      {
      return true;
      }
    }
  return false;
}

void cmForEachFunctionBlocker::
ScopeEnded(cmMakefile &mf) 
{
  cmSystemTools::Error("The end of a CMakeLists file was reached with a FOREACH statement that was not closed properly. Within the directory: ", 
                       mf.GetCurrentDirectory());
}

bool cmForEachCommand::InitialPass(std::vector<std::string> const& args)
{
  if(args.size() < 1)
    {
    this->SetError("called with incorrect number of arguments");
    return false;
    }
  
  // create a function blocker
  cmForEachFunctionBlocker *f = new cmForEachFunctionBlocker();
  if ( args.size() > 1 )
    {
    if ( args[1] == "RANGE" )
      {
      int start = 0;
      int stop = 0;
      int step = 0;
      if ( args.size() == 3 )
        {
        stop = atoi(args[2].c_str());
        }
      if ( args.size() == 4 )
        {
        start = atoi(args[2].c_str());
        stop = atoi(args[3].c_str());
        }
      if ( args.size() == 5 )
        {
        start = atoi(args[2].c_str());
        stop = atoi(args[3].c_str());
        step = atoi(args[4].c_str());
        }
      if ( step == 0 )
        {
        if ( start > stop )
          {
          step = -1;
          }
        else
          {
          step = 1;
          }
        }
      if ( 
        (start > stop && step > 0) ||
        (start < stop && step < 0) ||
        step == 0
        )
        {
        cmOStringStream str;
        str << "called with incorrect range specification: start ";
        str << start << ", stop " << stop << ", step " << step;
        this->SetError(str.str().c_str());
        return false;
        }
      std::vector<std::string> range;
      char buffer[100];
      range.push_back(args[0]);
      int cc;
      for ( cc = start; ; cc += step )
        {
        if ( (step > 0 && cc > stop) || (step < 0 && cc < stop) )
          {
          break;
          }
        sprintf(buffer, "%d", cc);
        range.push_back(buffer);
        if ( cc == stop )
          {
          break;
          }
        }
      f->Args = range;
      }
    else
      {
      f->Args = args;
      }
    }
  else
    {
    f->Args = args;
    }
  this->Makefile->AddFunctionBlocker(f);
  
  return true;
}

