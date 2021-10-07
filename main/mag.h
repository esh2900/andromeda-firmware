#ifndef MAGNETOMETRO_H
#define MAGNETOMETRO_H

#include "wmm.h"
#include "EGM9615.h"


double Declination(double lat, double _long,double alt, int day,int month, int year){
    double Decl=0;
    return Decl;
}

//Função que calcula o azimute do corpo com base na informação do magnetômetro
//recebe os parametros de posição e do acelerômetro
double azimut(double h_xmag,double h_ymag, double lat, double _long,double alt, int day,int month, int year){
    double azim=atan(h_ymag/h_xmag)+Declination(lat,_long,alt,day,month,year);
    return azim;
}

#endif