// This is a basic Flutter widget test.
//
// To perform an interaction with a widget in your test, use the WidgetTester
// utility in the flutter_test package. For example, you can send tap and scroll
// gestures. You can also use WidgetTester to find child widgets in the widget
// tree, read text, and verify that the values of widget properties are correct.

import 'package:flutter_test/flutter_test.dart';

import 'package:smart_parking_app/main.dart';

void main() {
  testWidgets('parking dashboard renders', (WidgetTester tester) async {
    await tester.pumpWidget(const SmartParkingApp());

    expect(find.textContaining('Smart Parking'), findsOneWidget);
    expect(find.text('SLOTS AVAILABLE'), findsOneWidget);
  });
}
