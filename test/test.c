void foo() {
	int a[1000];
	for (int i = 7; i*i < 1000; ++i) {
		a[i] = i;
	}
}

int bar() {
	int sum = 0;
	for (int j = 0; j < 100; j++) {
		sum += j;
	}
	return sum;
}

int main() {
	foo();
	bar();
}
