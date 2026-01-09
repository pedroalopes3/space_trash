#!/usr/bin/env python3
import zmq
import sys
import libconf

try:
    import messages_pb2
except ImportError:
    print("ERROR: messages_pb2 module not found!")
    print("Generate it with: protoc --proto_path=. --python_out=. messages.proto")
    sys.exit(1)


def main():
    """Connect to the publisher and listen for ScoreUpdate messages."""
    # Load config
    try:
        with open("universe-simulator.conf") as f:
            config = libconf.load(f)
            max_trash = config.get("max_trash", 50)  
            pubsub_endpoint = config.get("pubsub_endpoint", "tcp://127.0.0.1:5556") 
    except FileNotFoundError:
        print("ERROR: universe-simulator.conf not found!")
        sys.exit(1)
    except Exception as e:
        print(f"ERROR reading config: {e}")
        sys.exit(1)
    
    ctx = zmq.Context.instance()
    sock = ctx.socket(zmq.SUB)
    sock.connect(pubsub_endpoint)
    
    # Subscribe only to SCORE: topic
    sock.setsockopt(zmq.SUBSCRIBE, b"SCORE:")
    
    # Set receive timeout to 5 seconds (5000 ms)
    sock.setsockopt(zmq.RCVTIMEO, 5000)
    
    while True:
        try:

            payload = sock.recv()
            
            topic_prefix = b"SCORE:"
            if not payload.startswith(topic_prefix):
                continue
            
            msg_payload = payload[len(topic_prefix):]
            
            msg = messages_pb2.ScoreUpdate()
            try:
                msg.ParseFromString(msg_payload)
            except Exception:
                continue
            
            print("=" * 20)
            print("Dashboard")
            print("=" * 20)

            if len(msg.planet_scores) > 0:
                print("PLANETS (Recycled Trash)")
                for planet in msg.planet_scores:
                    name = planet.name if planet.HasField("name") else "Unknown"
                    score = planet.score if planet.HasField("score") else 0
                    print(f"    {name} - {score}")

            if len(msg.ships) > 0:
                print("TRASH-SHIPS (Trash Cargo)")
                for i, ship in enumerate(msg.ships):
                    ship_name = ship.name if ship.HasField("name") else f"Ship {i}"
                    capacity = ship.capacity if ship.HasField("capacity") else 0
                    print(f"    {ship_name} - {capacity}")

            
            if msg.HasField("trash_count"):
                print("UNIVERSE")
                print(f"    ROAMING TRASH {msg.trash_count}")
                trash_capacity_percentage = (msg.trash_count / max_trash) * 100
                print(f"    TRASH CAPACITY {trash_capacity_percentage:.1f}%")
            
            print()
            sys.stdout.flush()
            
        except KeyboardInterrupt:
            print("\n\nShutting down...")
            break
        except zmq.error.Again:
            print("\nError: No data received from server for 5 seconds")
            print("The server may be offline or not responding. Shutting down...")
            break
        except Exception as e:
            print(f"Error processing message: {e}")
            continue
    
    sock.close()
    ctx.term()


if __name__ == "__main__":
    main()
