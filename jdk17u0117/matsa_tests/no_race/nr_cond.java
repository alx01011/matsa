import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class nr_cond {
    private enum LightState { RED, GREEN }

    private LightState currentState = LightState.RED; // Initial state
    private final Lock lock = new ReentrantLock();
    private final Condition greenLight = lock.newCondition();

    // Method to wait for the green light (Cars)
    public void waitForGreen(String carName) throws InterruptedException {
        lock.lock();
        try {
            while (currentState != LightState.GREEN) {
                System.out.println(carName + " is waiting for the green light...");
                greenLight.await();
            }
            System.out.println(carName + " is passing the green light.");
        } finally {
            lock.unlock();
        }
    }

    // Method to change the light state (Traffic Light Controller)
    public void changeLight() throws InterruptedException {
        lock.lock();
        try {
            System.out.println("Traffic light is changing to GREEN...");
            currentState = LightState.GREEN;
            greenLight.signalAll(); // Signal all waiting cars
        } finally {
            lock.unlock();
        }
    }

    public static void main(String[] args) {
        nr_cond trafficLight = new nr_cond();

        // Car threads waiting for the green light
        Runnable car = (name) -> {
            try {
                trafficLight.waitForGreen((String) name);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        };

        Thread car1 = new Thread(() -> car.run("Car 1"));
        Thread car2 = new Thread(() -> car.run("Car 2"));
        Thread car3 = new Thread(() -> car.run("Car 3"));

        car1.start();
        car2.start();
        car3.start();

        // Simulate a delay before changing the light to green
        Thread trafficController = new Thread(() -> {
            try {
                Thread.sleep(2000); // Simulate traffic light timer
                trafficLight.changeLight();
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        });

        trafficController.start();

        try {
            car1.join();
            car2.join();
            car3.join();
            trafficController.join();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
        }

        System.out.println("All cars have passed the traffic light.");
    }
}
