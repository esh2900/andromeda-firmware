#ifndef ACEL_H
#define ACEL_H
#include<math.h>

//Calculam as orientações medidas pelos sensores
//Recebem os valores medidos por eles
double alt(double X_acel,double Y_acel, double Z_acel){ //Calcula a altitude vista pelo sensor
    double alt = atan(-1 * X_acel / sqrt(pow(Y_acel, 2) + pow(Z_acel, 2))) * 180 / M_PI;
return alt;
}

double inc(double X_acel,double Y_acel, double Z_acel){//Calcula a inclinação da estrutura, com relação ao chão
    double inc = atan(Y_acel / sqrt(pow(X_acel, 2) + pow(Z_acel, 2))) * 180 / M_PI;
return inc;
}


//calcula os valores filtrados dos sinais do acelerômetro
//Recebe o valor da ultima filtragem e a medida atual do sensor
//Inicializar em 0 caso não tenha uma filtragem anterior
double low_pass_alt(double prev,double medida){ //Gera o resultado filtrado do valor medido
    if(!isnan(medida)){
        double filter= 0.92*prev + 0.08*medida;
        return filter;
    }
    else{
        return prev;
    }
}

//Testa a necessidade de envio de alerta, 0=ok,1=Alerta,2=Emergência
//Recebe a inclinação medida do par de acelerômetros e a altitude do
//acelerometro no magnetômetro
int warning_test(double inc_1,double inc_2,double alt_2){
    if(abs(inc_2)>15.0 || abs(alt_2)>15.0){//tripe muito inclinado
        return 1;
    }
    else if(inc_1-inc_2>90.0  || inc_1-inc_2<0.0){//estrutura no limite de movimento
        return 1;
    }
    else if(abs(inc_2)>45.0 || abs(alt_2)>45.0){//possível queda
        return 2;
    }
    else{//operação normal
        return 0;
    }

}

//Função que encontra o viés médio do sesor e devolve o fator de correção
//recebe o endereço uma série de medidas sequenciais do sensor e o número de medidas
double get_bias(double *measure_map,int n){
    double valid=0;
    double sum=0;
    for(int i=0;i<n;i++){
        if(!isnan(measure_map[i])){
            valid++;
            sum+=measure_map[i];
        }
    }
    return sum/valid;
}

#endif