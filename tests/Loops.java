public class Loops {
    public static void main(String[] args) {
        // for loop sum
        int sum = 0;
        for (int i = 1; i <= 5; i++) {
            sum += i;
        }
        System.out.println(sum);

        // while loop countdown
        int n = 3;
        while (n > 0) {
            System.out.println(n);
            n--;
        }
    }
}
