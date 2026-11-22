import { ChangeDetectorRef, Component, OnDestroy, OnInit, inject } from '@angular/core';
import { CommonModule } from '@angular/common';
import mqtt, { MqttClient } from 'mqtt';

interface Medicion {
  distancia: number;
  nivel: number; 
  tanque: number; 
  valvula: string;
  hora: string;
}

@Component({
  selector: 'app-root',
  standalone: true,
  imports: [CommonModule],
  templateUrl: './app.html',
  styleUrl: './app.css'
})
export class App implements OnInit, OnDestroy {
  private brokerUrl = 'wss://g11dab68.ala.us-east-1.emqxsl.com:8084/mqtt';
  private usuario = 'tanque';
  private password = '1234';

  private topicSensor = 'esp32/casa01/sensor';
  private topicComando = 'esp32/casa01/comando';

  private client?: MqttClient;
  private cdr = inject(ChangeDetectorRef);

  conectadoBroker = false;
  distanciaActual: number | null = null;
  nivelAgua: number = 0; 
  porcentajeTanque: number = 0; 
  estadoValvula: string = 'Esperando...'; 
  ultimaActualizacion = '--';
  
  historial: Medicion[] = [];

  ngOnInit(): void {
    this.conectarMQTT();
  }

  conectarMQTT(): void {
    this.client = mqtt.connect(this.brokerUrl, {
      username: this.usuario,
      password: this.password,
      protocolVersion: 4,
      clean: true,
      clientId: 'ANGULAR_DASH_' + Math.random().toString(16).substring(2, 8)
    });

    this.client.on('connect', () => {
      this.conectadoBroker = true;
      this.cdr.markForCheck();
      this.client?.subscribe(this.topicSensor);
    });

    this.client.on('close', () => {
      this.conectadoBroker = false;
      this.cdr.markForCheck();
    });

    this.client.on('message', (topic, payload) => {
      if (topic === this.topicSensor) {
        try {
          const datos = JSON.parse(payload.toString());
          
          this.distanciaActual = datos.distancia;
          this.nivelAgua = datos.nivel;
          this.porcentajeTanque = datos.tanque;
          this.estadoValvula = datos.valvula;
          this.ultimaActualizacion = new Date().toLocaleTimeString();

          this.historial.unshift({
            distancia: datos.distancia,
            nivel: datos.nivel,
            tanque: datos.tanque,
            valvula: datos.valvula,
            hora: this.ultimaActualizacion
          });
          if (this.historial.length > 20) this.historial.pop();

          this.cdr.markForCheck();
        } catch (e) {
          console.error('Error al leer JSON', e);
        }
      }
    });
  }

  controlarValvula(accion: string): void {
    if (!this.client?.connected) return;
    const mensaje = JSON.stringify({ accion: accion });
    this.client.publish(this.topicComando, mensaje, { qos: 0 });
  }

  get puntosGrafica(): string {
    if (this.historial.length < 2) return '';
    const datos = [...this.historial].reverse();
    
    return datos.map((dato, index) => {
      const x = 20 + (index / (datos.length - 1)) * 560;
      const y = 180 - (dato.tanque / 100) * 160; 
      return `${x},${y}`;
    }).join(' ');
  }

  ngOnDestroy(): void {
    this.client?.end();
  }
}