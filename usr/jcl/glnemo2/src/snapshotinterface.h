// ============================================================================
// Copyright Jean-Charles LAMBERT - 2007-2008                                  
// e-mail:   Jean-Charles.Lambert@oamp.fr                                      
// address:  Dynamique des galaxies                                            
//           Laboratoire d'Astrophysique de Marseille                          
//           P�le de l'Etoile, site de Ch�teau-Gombert                         
//           38, rue Fr�d�ric Joliot-Curie                                     
//           13388 Marseille cedex 13 France                                   
//           CNRS U.M.R 6110                                                   
// ============================================================================
// See the complete license in LICENSE and/or "http://www.cecill.info".        
// ============================================================================
/**
	@author Jean-Charles Lambert <Jean-Charles.Lambert@oamp.fr>
 */

#ifndef GLNEMOSNAPSHOTINTERFACE_H
#define GLNEMOSNAPSHOTINTERFACE_H
#include <iostream>
#include <assert.h>
#include <QFile>
#include "particlesobject.h"
#include "particlesdata.h"
#include "componentrange.h"

namespace glnemo {
//class ParticlesData;

class SnapshotInterface
{
  public:
    SnapshotInterface()
    {
      obj=NULL; pos=NULL; vel=NULL;
      part_data=NULL;
      end_of_data=false;
      first=true;
      crv.clear();
    };
    virtual ~SnapshotInterface() {};
    // ---------------------------------------------------
    // Pure Virtual functions, *** MUST be implemented ***
    // ---------------------------------------------------
    virtual SnapshotInterface * newObject(const std::string _filename) = 0 ;
    virtual bool isValidData() = 0;
    virtual ComponentRangeVector * getSnapshotRange() = 0;
    virtual int initLoading(const bool _load_vel, const std::string _select_time) = 0;
    virtual int nextFrame(const int * index_tab, const int nsel)= 0;
           // index_tab = array of selected indexes (size max=part_data->nbody)
           // nsel      = #particles selected in index_tab                     
           // particles not selected must have the value '-1'                  
    virtual int close() = 0;
    virtual QString endOfDataMessage() = 0;
    // simple Virtual functions
    virtual bool isEndOfData() const { return end_of_data;};
    // normal functions        
    void setFileName(std::string _f) { filename = _f;};
    std::string getFileName() const { return filename;};
    std::string getInterfaceType() { return interface_type;};
    bool isFileExist() { return QFile::exists(QString(filename.c_str()));};
    ParticlesData * part_data;
    int nbody_first;
    float time_first;
    ComponentRangeVector crv_first;
  protected:
    SnapshotInterface * obj;
    std::string filename;
    std::string interface_type;
    mutable bool end_of_data;
    std::string select_part, select_time;
    bool keep_all, load_vel;
    ComponentRangeVector crv;
    float * pos, *vel;
    bool first;

};

}
Q_DECLARE_INTERFACE(glnemo::SnapshotInterface,
                    "Glnemo.SnapshotInterface/1.0");
#endif