/*=========================================================================

  Program:   ParaView
  Module:    vtkSMRepresentedArrayListDomain.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSMRepresentedArrayListDomain.h"

#include "vtkCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPVDataInformation.h"
#include "vtkPVRepresentedArrayListSettings.h"
#include "vtkSMProperty.h"
#include "vtkSMRepresentationProxy.h"

#include <vtksys/RegularExpression.hxx>

// Callback to update the RepresentedArrayListDomain
class vtkSMRepresentedArrayListDomainUpdateCommand : public vtkCommand
{
public:
  vtkWeakPointer<vtkSMRepresentedArrayListDomain> Domain;
  typedef vtkCommand Superclass;
  vtkSMRepresentedArrayListDomainUpdateCommand()
    {
      this->Domain = NULL;
    }
  virtual const char* GetClassNameInternal() const
    { return "vtkSMRepresentedArrayListDomainUpdateCommand"; }
  static vtkSMRepresentedArrayListDomainUpdateCommand* New()
    {
      return new vtkSMRepresentedArrayListDomainUpdateCommand();
    }
  virtual void Execute(vtkObject*, unsigned long, void*)
  {
    if (this->Domain)
      {
      this->Domain->Update(NULL);
      }
  }
};


vtkStandardNewMacro(vtkSMRepresentedArrayListDomain);
//----------------------------------------------------------------------------
vtkSMRepresentedArrayListDomain::vtkSMRepresentedArrayListDomain()
{
  this->ObserverId = 0;
  // The question is whether to just pick the first available array when the
  // "chosen" attribute is not available or not. Opting for not. This keeps us
  // from ending up scalar coloring random data arrays by default. This logic
  // may need to be reconsidered.
  this->PickFirstAvailableArrayByDefault = false;

  // Set up observer on vtkPVRepresentedArrayListSettings so that the domain
  // updates whenever the settings are changed.
  vtkSMRepresentedArrayListDomainUpdateCommand* observer =
    vtkSMRepresentedArrayListDomainUpdateCommand::New();
  observer->Domain = this;

  vtkPVRepresentedArrayListSettings* arrayListSettings = vtkPVRepresentedArrayListSettings::GetInstance();
  arrayListSettings->AddObserver(vtkCommand::ModifiedEvent, observer);
  observer->FastDelete();
}

//----------------------------------------------------------------------------
vtkSMRepresentedArrayListDomain::~vtkSMRepresentedArrayListDomain()
{
  if (this->RepresentationProxy && this->ObserverId)
    {
    this->RepresentationProxy->RemoveObserver(this->ObserverId);
    this->ObserverId = 0;
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::Update(vtkSMProperty* property)
{
  // When the update happens the first time, save a reference to the
  // representation proxy and add observers so that we can monitor the
  // representation updates.
  vtkSMRepresentationProxy* selfProxy = (this->GetProperty()?
    vtkSMRepresentationProxy::SafeDownCast(this->GetProperty()->GetParent()) : NULL);
  this->SetRepresentationProxy(selfProxy);
  this->Superclass::Update(property);
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::SetRepresentationProxy(
  vtkSMRepresentationProxy* repr)
{
  if (this->RepresentationProxy != repr)
    {
    if (this->RepresentationProxy && this->ObserverId)
      {
      this->RepresentationProxy->RemoveObserver(this->ObserverId);
      }
    this->RepresentationProxy = repr;
    if (repr)
      {
      this->ObserverId = this->RepresentationProxy->AddObserver(
        vtkCommand::UpdateDataEvent,
        this,
        &vtkSMRepresentedArrayListDomain::OnRepresentationDataUpdated);
      }
    }
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::OnRepresentationDataUpdated()
{
  this->Update(NULL);
}

//----------------------------------------------------------------------------
bool vtkSMRepresentedArrayListDomain::IsFilteredArrayName(const char* name)
{
  vtkPVRepresentedArrayListSettings* colorArraySettings = vtkPVRepresentedArrayListSettings::GetInstance();
  for (int idx = 0; idx < colorArraySettings->GetNumberOfFilterExpressions(); ++idx)
    {
    std::string filterExpression = colorArraySettings->GetFilterExpression(idx);
    vtksys::RegularExpression re(filterExpression);
    bool matches = re.find(name);
    if (matches)
      {
      return true;
      }
    }

  return false;
}

//----------------------------------------------------------------------------
vtkPVDataInformation* vtkSMRepresentedArrayListDomain::GetExtraDataInformation()
{
  return this->RepresentationProxy?
    this->RepresentationProxy->GetRepresentedDataInformation() : NULL;
}

//----------------------------------------------------------------------------
void vtkSMRepresentedArrayListDomain::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
