import zmq
import messages_pb2 
import os
import time
import sys

def main():
    
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    server_address = "tcp://127.0.0.1:5556"
    print(f"Connecting to universe server: {server_address}")
    socket.connect(server_address)
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    max_trash_limit = 50 

    print("Waiting for universe updates...")

    while True:
        try:
        
            msg_data = socket.recv()

            update = messages_pb2.UpdateMessage()
            update.ParseFromString(msg_data)
            
            os.system('cls' if os.name == 'nt' else 'clear')

            print("========================================")
            print("===      UNIVERSE DASHBOARD          ===")
            print("========================================\n")

            # Planets
            print("PLANETS (Recycled Trash):")
            print("-" * 30)
            
            sorted_planets = sorted(update.planets, key=lambda p: p.planet_index)
            
            for p_update in sorted_planets:
                
                p_name = chr(ord('A') + p_update.planet_index)
                is_recycle_str = " [RECYCLE CENTER]" if p_update.isrecycle else ""
                count = p_update.recycled_count if p_update.HasField('recycled_count') else 0
                print(f" Planeta {p_name}: {count:3d} items recycled {is_recycle_str}")

            print("\n")

            # ships
            print("TRASH SHIPS (Current Cargo):")
            print("-" * 30)
            
            active_ships_count = 0
            
            for ship in update.ships:
                if ship.ID != 0: 
                    active_ships_count += 1
                    
                    cargo = ship.cargo if ship.HasField('cargo') else 0
                    
                    print(f" Ship ID {ship.ID:<6} | Cargo: {cargo:2d}")
            
            if active_ships_count == 0:
                print(" -> No active ships in the universe.")

            print("\n")

            # Universe
            print("UNIVERSE:")
            print("-" * 30)
            
            roaming_trash = len(update.trash)
            capacity_percent = (roaming_trash / max_trash_limit) * 100
            
            print(f" Roaming Trash: {roaming_trash}")
            
            bar_length = 20
            filled_length = int(bar_length * roaming_trash // max_trash_limit)
            bar = '█' * filled_length + '-' * (bar_length - filled_length)
            
            print(f" Trash Capacity: |{bar}| {capacity_percent:.1f}%")
            
            if capacity_percent >= 100:
                print("\n!!! CRITICAL WARNING: UNIVERSE COLLAPSE IMMINENT !!!")
            elif capacity_percent > 80:
                print("\n! WARNING: Trash levels critically high !")

        except KeyboardInterrupt:
            print("\nDashboard closed by user.")
            break
        except zmq.ZMQError as e:
            print(f"\nZMQ Error: {e}")
            break
        except Exception as e:
            print(f"\nError processing message: {e}")
            time.sleep(1)

if __name__ == "__main__":
    main()