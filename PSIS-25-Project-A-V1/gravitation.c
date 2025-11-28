void new_trash_acceleration(planet_stucture planets[], int total_planets,
                            trash_stucture trash[], int total_trash){
    vector total_vector_force;


    for (int n_trash = 0; n_trash < total_trash; n_trash ++){
        total_vector_force.amplitude = 0;
        total_vector_force.angle = 0;
        for (int n_planet = 0; n_planet < total_planets; n_planet ++){
            float force_vector_x = planets[n_planet].x - trash[n_trash].x;
            float force_vector_y = planets[n_planet].y - trash[n_trash].y;
            vector local_vector_force = make_vector(force_vector_x, force_vector_y);
            local_vector_force.amplitude = (planets[n_planet].mass * trash[n_trash].mass)/
                                            pow(local_vector_force.amplitude, 2);
            total_vector_force = add_vectors(local_vector_force, total_vector_force);
        }
        trash[n_trash].acceleration = total_vector_force ; // / trash[n_trash].mass
    }
}


void new_trash_velocity(trash_stucture trash[], int total_trash){
    for (int n_trash = 0; n_trash < total_trash; n_trash ++){
        trash[n_trash].velocity.amplitude *= 0.99; //friction
        trash[n_trash].velocity = add_vectors(trash[n_trash].velocity, trash[n_trash].acceleration);
    }
}

void new_trash_position( trash_stucture trash[], int total_trash){
    for (int n_trash = 0; n_trash < total_trash; n_trash ++){
        trash[n_trash].x += trash[n_trash].velocity.amplitude * cos( trash[n_trash].velocity.angle);
        trash[n_trash].y += trash[n_trash].velocity.amplitude * sin( trash[n_trash].velocity.angle);;
        correct_position(&trash[n_trash].x);
        correct_position(&trash[n_trash].y);
    }
}
