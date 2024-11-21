#include "Bullet.h"
#include <cmath>
#include <cstdlib>  // Para rand()

// Constructor de la clase Bullet. Inicializa la bala con la posición inicial, objetivo, y calcula la dirección.
Bullet::Bullet(int startX, int startY, int targetX, int targetY, int shooterId)
    : x(startX), y(startY), targetX(targetX), targetY(targetY), shooterId(shooterId) {
    // Calcular la dirección inicial hacia el objetivo
    calculateDirection();
}

// Calcula la dirección de la bala hacia el objetivo
void Bullet::calculateDirection() {
    float dx = targetX - x;
    float dy = targetY - y;
    float length = std::sqrt(dx * dx + dy * dy);
    direccionX = dx / length;
    direccionY = dy / length;
}

             // Cálculo de la dirección: Este método calcula la dirección en la que la bala debe
             // moverse. dx y dy son las diferencias en las coordenadas X e Y entre la posición actual 
             // y el objetivo. Luego se normaliza la dirección dividiendo por la longitud (distancia) entre esos 
             // puntos.

// Método para actualizar la bala. Mueve la bala, verifica colisiones y rebotes en obstáculos y bordes del mapa.
void Bullet::update(const Map& map, std::vector<Tank>& tanks, bool& destroyBullet) {

    // Avanzar la bala en la dirección calculada
    float newX = x + direccionX * speed;
    float newY = y + direccionY * speed;

    // Verificar si hay un obstáculo en la nueva posición
    if (map.isObstacle(static_cast<int>(newX), static_cast<int>(newY))) {
        // Si hay un obstáculo, la bala rebota de forma aleatoria
        float randomAngle = (std::rand() % 90 - 45) * (M_PI / 180.0f);
        float newDirX = direccionX * std::cos(randomAngle) - direccionY * std::sin(randomAngle);
        float newDirY = direccionX * std::sin(randomAngle) + direccionY * std::cos(randomAngle);
        direccionX = newDirX;
        direccionY = newDirY;
    } else {
        // Si no hay obstáculos, actualizar la posición de la bala
        x = newX;
        y = newY;
    }

    // Verificar si colisiona con algún tanque. Si impacta contra un tanque, se aplica el daño y la bala se destruye.
    for (Tank& tank : tanks) {
        if (tank.getX() == static_cast<int>(x) && tank.getY() == static_cast<int>(y)) {
            if (tank.getId() != shooterId) {
                // Aplica el daño correcto según el tipo de tanque. Los tanques azul/celeste reciben 25% de daño, y los rojo/amarillo 50%.
                switch (tank.getColor()) {
                    case Tank::BLUE:
                    case Tank::CYAN:
                        tank.receiveDamage(25);  // 25% de daño para azul/celeste
                        break;
                    case Tank::RED:
                    case Tank::YELLOW:
                        tank.receiveDamage(50);  // 50% de daño para rojo/amarillo
                        break;
                }
                destroyBullet = true;  // Destruir la bala tras impactar
                return;
            }
        }
    }

    // Verifica si la bala sale de los bordes de la pantalla. Si llega a los bordes, rebota.
    if (x < 0 || y < 0 || x >= map.getSize() || y >= map.getSize()) {
        
        if (x < 0 || x >= map.getSize()) {
            direccionX = -direccionX;  // Invierte dirección en el eje X
        }
        if (y < 0 || y >= map.getSize()) {
            direccionY = -direccionY;  // Invierte dirección en el eje Y
        }
    }
}

// Método para dibujar la bala
void Bullet::draw(sf::RenderWindow& window, int cellSize) {
    sf::CircleShape bulletShape(cellSize / 6);  // Tamaño de la bala
    bulletShape.setFillColor(sf::Color::Black); // Color de la bala
    bulletShape.setPosition(x * cellSize, y * cellSize);
    window.draw(bulletShape);
}




