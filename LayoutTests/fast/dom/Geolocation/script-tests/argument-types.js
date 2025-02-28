description("Tests the acceptable types for arguments to Geolocation methods.");

function shouldNotThrow(expression)
{
  try {
    eval(expression);
    testPassed(expression + " did not throw exception.");
  } catch(e) {
    testFailed(expression + " should not throw exception. Threw exception " + e);
  }
}

function test(expression, expressionShouldThrow, expectedException) {
    if (expressionShouldThrow) {
        if (expectedException)
            shouldThrow(expression, '(function() { return "' + expectedException + '"; })();');
        else
            shouldThrow(expression, '(function() { return "Error: TYPE_MISMATCH_ERR: DOM Exception 17"; })();');
    } else {
        shouldNotThrow(expression);
    }
}

var emptyFunction = function() {};

function ObjectThrowingException() {};
ObjectThrowingException.prototype.valueOf = function() {
    throw new Error('valueOf threw exception');
}
ObjectThrowingException.prototype.__defineGetter__("enableHighAccuracy", function() {
    throw new Error('enableHighAccuracy getter exception');
});
var objectThrowingException = new ObjectThrowingException();


test('navigator.geolocation.getCurrentPosition()', true);

test('navigator.geolocation.getCurrentPosition(undefined)', true);
test('navigator.geolocation.getCurrentPosition(null)', true);
test('navigator.geolocation.getCurrentPosition({})', true);
test('navigator.geolocation.getCurrentPosition(objectThrowingException)', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction)', false);
//test('navigator.geolocation.getCurrentPosition(Math.abs)', false);
test('navigator.geolocation.getCurrentPosition(true)', true);
test('navigator.geolocation.getCurrentPosition(42)', true);
test('navigator.geolocation.getCurrentPosition(Infinity)', true);
test('navigator.geolocation.getCurrentPosition(-Infinity)', true);
test('navigator.geolocation.getCurrentPosition("string")', true);

test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, null)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, {})', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction, objectThrowingException)', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction, emptyFunction)', false);
//test('navigator.geolocation.getCurrentPosition(emptyFunction, Math.abs)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, true)', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction, 42)', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction, Infinity)', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction, -Infinity)', true);
test('navigator.geolocation.getCurrentPosition(emptyFunction, "string")', true);

test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, undefined)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, null)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, objectThrowingException)', true, 'Error: enableHighAccuracy getter exception');
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, emptyFunction)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, true)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, 42)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, Infinity)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, -Infinity)', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, "string")', false);

test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:undefined})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:null})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:{}})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:objectThrowingException})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:emptyFunction})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:true})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:42})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:-Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {dummyProperty:"string"})', false);

test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:undefined})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:null})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:{}})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:objectThrowingException})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:emptyFunction})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:true})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:42})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:-Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {enableHighAccuracy:"string"})', false);

test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:undefined})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:null})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:{}})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:objectThrowingException})', true, 'Error: valueOf threw exception');
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:emptyFunction})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:true})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:42})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:-Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {maximumAge:"string"})', false);

test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:undefined})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:null})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:{}})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:objectThrowingException})', true, 'Error: valueOf threw exception');
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:emptyFunction})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:true})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:42})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:-Infinity})', false);
test('navigator.geolocation.getCurrentPosition(emptyFunction, undefined, {timeout:"string"})', false);

window.jsTestIsAsync = false;
window.successfullyParsed = true;

