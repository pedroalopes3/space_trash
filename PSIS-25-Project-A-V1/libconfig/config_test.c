#include <libconfig.h>

int main(){

    config_t cfg;
    config_init(&cfg);

    if(!config_read_file(&cfg, "example.conf"))
    {
        fprintf(stderr, "Config file error\n");
        config_destroy(&cfg);
        return 1;  
    }
    
    int missing_parameter;
    if(config_lookup_int(&cfg, "missing_parameter", &missing_parameter)== CONFIG_FALSE){
        printf("Configuration not found\n");
    }
    int parametro_1_int;
    config_lookup_int(&cfg, "parametro_i_int", &parametro_1_int);
    printf("Max parametro_1_float: %d\n", parametro_1_int);

    double parametro_2_float;
    config_lookup_float(&cfg, "parametro_2_float", &parametro_2_float);
    printf("parametro_2_float: %f\n", parametro_2_float);  



    char * parametro_3_string;
    config_lookup_string(&cfg, "parametro_3_string", &parametro_3_string);
    printf("parametro_3_string: %s\n", parametro_3_string);  

    config_destroy(&cfg);

    return 0;
}