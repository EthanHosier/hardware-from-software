#!/usr/bin/env python3
"""Original schematic-style wiring diagrams. Positions are not breadboard holes."""
from pathlib import Path
from html import escape as e
ROOT=Path(__file__).resolve().parents[1]
def text(x,y,s,size=17,fill='#17263b'):
    return f'<text x="{x}" y="{y}" font-family="system-ui,sans-serif" font-size="{size}" fill="{fill}">{e(s)}</text>'
def base(title,subtitle,height=500):
    return [f'<svg xmlns="http://www.w3.org/2000/svg" width="1100" height="{height}" viewBox="0 0 1100 {height}" role="img"><title>{e(title)}</title><desc>{e(subtitle)}</desc><rect width="1100" height="{height}" fill="#f6f8fc"/>',text(35,40,title,25),text(35,70,subtitle,15)]
def box(parts,x,y,w,h,label):
    parts.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="12" fill="white" stroke="#506b85" stroke-width="2"/>');parts.append(text(x+18,y+32,label,21))
def line(parts,x1,y1,x2,y2,label,color='#286ba3',offset=0):
    parts.append(f'<path d="M{x1} {y1} L{x2} {y2}" stroke="{color}" stroke-width="3" fill="none"/>');parts.append(text(min(x1,x2)+15,(y1+y2)/2-10+offset,label,16,color))
def save(name,p):
    p.append('</svg>');(ROOT/'src/assets'/f'circuit-{name}.svg').write_text('\n'.join(p))
p=base('External LED on Pico 2','Schematic, not a breadboard placement diagram. USB connector defines the board’s top.',440)
box(p,35,110,280,230,'Pico 2')
p += [text(55,190,'GP16 · physical 21'),text(55,300,'GND · physical 23')]
line(p,315,180,435,180,'output')
box(p,435,145,150,70,'1 kΩ')
line(p,585,180,670,180,'')
box(p,670,135,360,105,'LED')
p += [text(690,205,'anode (+) ←   → cathode (−)',17),text(35,395,'Current: GP16 → resistor → LED anode → LED cathode → GND. No 5V connection.',17)]
p.append('<path d="M960 240 V295 H315" fill="none" stroke="#465363" stroke-width="3"/>')
save('pico-led',p)
p=base('Active-low button on Pico 2','Use an external 4.7 kΩ pull-up. Button connection pairs are explained in Chapter 0.',480)
box(p,35,110,330,270,'Pico 2')
p += [text(55,175,'3V3(OUT) · physical 36'),text(55,255,'GP17 input · physical 22'),text(55,340,'GND · physical 23')]
line(p,365,165,520,165,'')
box(p,520,130,180,70,'4.7 kΩ')
p.append('<path d="M700 165 H790 V245 H365 M790 245 V300" stroke="#286ba3" stroke-width="3" fill="none"/>')
p += [text(800,245,'idle HIGH',17),text(800,270,'press → LOW',17)]
box(p,700,300,230,65,'Push button')
p.append('<path d="M700 335 H365" stroke="#465363" stroke-width="3" fill="none"/>')
p.append(text(35,435,'Released: pull-up defines HIGH. Pressed: switch joins GP17 to ground. Keep Pico internal pulls disabled.',16))
save('pico-button',p)
for name,title,left,right,rows,note in [
 ('i2c','I²C: Pi controller, Pico peripheral','Pi 4 header','Pico 2 header',[
 ('SDA · BCM2 · pin3','SDA · GP4 · pin6','SDA bidirectional'),('SCL · BCM3 · pin5','SCL · GP5 · pin7','SCL controller clock'),('GND · pin6','GND · pin8','common signal ground')], 'Pi SDA/SCL pull-ups go to 3.3V. Each board has its own USB supply; do not join power rails.'),
 ('uart','UART: cross transmit and receive','Pi 4 header','Pico 2 header',[
 ('TX · BCM14 · pin8','RX · GP1 · pin2','Pi → Pico'),('RX · BCM15 · pin10','TX · GP0 · pin1','Pico → Pi'),('GND · pin6','GND · pin8','common signal ground')], 'Both ends: 115200 baud, 8N1, 3.3V logic. Power off both boards before changing wires.')]:
 p=base(title,'Schematic wiring map. Pin numbers are physical header positions.',470)
 box(p,30,110,350,260,left);box(p,720,110,350,260,right)
 for i,(a,b,label) in enumerate(rows):
  y=190+i*65;p += [text(50,y,a,17),text(740,y,b,17)]
  line(p,380,y-5,720,y-5,label,'#465363' if i==2 else '#286ba3')
 p += [text(35,425,note,15)]
 save(name,p)
p=base('Pi 4: dedicated button and LED','Physical pin numbers refer to the 40-pin header. This circuit is separate from the Pico button.',590)
box(p,35,110,400,365,'Pi 4 header')
for y,label in [(175,'3V3 · physical17'),(260,'BCM27 input · physical13'),(345,'BCM17 output · physical11'),(440,'GND · physical14 / physical9')]:p.append(text(55,y,label,18))
box(p,540,130,170,65,'4.7 kΩ');box(p,820,225,230,65,'Button')
p.append('<path d="M435 165 H540 M710 165 H780 V250 H435 M780 250 H820 M1050 260 V435 H435" stroke="#286ba3" stroke-width="3" fill="none"/>')
box(p,540,310,170,65,'1 kΩ');box(p,820,310,230,65,'LED + → −')
p.append('<path d="M435 340 H540 M710 340 H820 M1050 345 V435" stroke="#286ba3" stroke-width="3" fill="none"/>')
p += [text(35,525,'Button pressed: physical voltage LOW; GPIO_ACTIVE_LOW translates this to logical pressed=true.',16),text(35,555,'LED current flows from BCM17 through 1 kΩ and the LED to GND. Power down before rewiring.',16)]
save('pi-button',p)
p=base('Capstone: physical button to a remote LED','Keep the Pi button/LED circuit and Pico LED circuit; join only UART signals and common ground.',500)
for x,label in [(30,'Pi button'),(305,'Pi 4 + Linux'),(720,'Pico 2')]:box(p,x,130,245,180,label)
p += [text(45,210,'BCM27 · pin13',17),text(325,210,'/dev/mybutton',18),text(325,245,'C++ → /dev/serial0',16),text(740,210,'UART0 GP0/GP1',17),text(740,245,'LED GP16 · pin21',17)]
line(p,275,220,305,220,'',offset=0)
line(p,550,195,720,195,'TX → RX',offset=-2)
line(p,550,255,720,255,'RX ← TX',offset=-2)
line(p,550,300,720,300,'GND ↔ GND','#465363')
p += [text(35,380,'Pi TX BCM14 pin8 → Pico RX GP1 pin2; Pi RX BCM15 pin10 ← Pico TX GP0 pin1.',17),text(35,415,'Pi GND pin6 ↔ Pico GND pin8. Pi and Pico remain powered separately; no power-rail wire.',17),text(35,455,'See the separate component circuits for resistor placement, button pull-up and LED polarity.',16)]
save('capstone',p)
