/*=========================================================================

  Program:   ITK Program Factory
  Module:    $RCSfile: $
  Language:  C++
  Date:      $Date: $
  Version:   $Revision: $

  Copyright (c) INRIA Saclay Île-de-France, Parietal Research Team. All rights reserved.
  See CodeCopyright.txt for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkCommandObjectFactory.h"

#include "itkMutexLock.h"
#include "itkMutexLockHolder.h"

#include <algorithm>
#include <string.h>

namespace itk
{

CommandObjectBase::Pointer
CommandObjectFactory::CreateCommandObject(const char* name)
{

  std::list<CommandObjectBase::Pointer> possibleCommandObject;
  std::list<LightObject::Pointer> allobjects =
    ObjectFactoryBase::CreateAllInstance("itkCommandObjectBase");
  for(std::list<LightObject::Pointer>::iterator i = allobjects.begin();
      i != allobjects.end(); ++i)
    {
    CommandObjectBase* command = dynamic_cast<CommandObjectBase*>(i->GetPointer());
    if(command)
      {
      possibleCommandObject.push_back(command);
      }
    else
      {
      std::cerr << "Error CommandObject factory did not return an CommandObjectBase: "
                << (*i)->GetNameOfClass()
                << std::endl;
      }
    }
  for(std::list<CommandObjectBase::Pointer>::iterator k = possibleCommandObject.begin();
      k != possibleCommandObject.end(); ++k)
    {
      if( strcmp((*k)->GetCommandName(), name)==0 )
        return *k;
    }
  return 0;
}

	
void CommandObjectFactory::PrintHelp(std::ostream &os, Indent indent)
{
  std::list<LightObject::Pointer> allobjects =
    ObjectFactoryBase::CreateAllInstance("itkCommandObjectBase");

  std::vector<std::string> commandNames;
  
  for(std::list<LightObject::Pointer>::iterator i = allobjects.begin();
      i != allobjects.end(); ++i)
  {
    CommandObjectBase* command = dynamic_cast<CommandObjectBase*>(i->GetPointer());
    if(command)
    {
      std::string line;
      line += command->GetCommandName();
      if (line.size() < 7)  line += "\t";
      if (line.size() < 16) line += "\t";
      
      line += " :: \t";
      line += command->GetShortDescription();
      
      commandNames.push_back ( line );
    }
  }

  std::sort (commandNames.begin(), commandNames.end());
  for (unsigned int i=0; i<commandNames.size(); i++)
  {
    os << indent << "\t " << commandNames[i] << std::endl;
    /*
      << "...";
      os << indent << command->GetShortDescription() << "\n";
      os << "\n";
    */
  }  
}
	
	
} // end namespace itk
