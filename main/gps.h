#ifndef GPS_H
#define GPS_H


struct GPS_data{ //struct com as informações do sensor
    double lat; //latitue
    double _long; //longitude
    double alt; //altitude
};

//Recebe uma série de dados de posição e a quantidade deles e devolve a posição média
//calculada para o momento
struct GPS_data get_avg_pos(struct GPS_data *data, int n){
    struct GPS_data avg_pos;

    for(int i=0;i<n;i++){
        avg_pos.lat+=data[i].lat;
        avg_pos._long+=data[i]._long;
        avg_pos.alt+=data[i].alt;
    }

    avg_pos.lat/=n;
    avg_pos._long/=n;
    avg_pos.alt/=n;
    return avg_pos;
}


#endif