function dateDiffInDays(a, b, _MS_PER_DAY) {
    // Discard the time and time-zone information.
    var utc1 = Date.UTC(a.getFullYear(), a.getMonth(), a.getDate());
    //print(a)
    //print(b)
    var utc2 = Date.UTC(b.getFullYear(), b.getMonth(), b.getDate());

    var result = Math.floor((utc2 - utc1) / _MS_PER_DAY);
    //print(result)
    return result
}


function removeA(arr) {
    var what, a = arguments, L = a.length, ax;
    while (L > 1 && arr.length) {
        what = a[--L];
        while ((ax= arr.indexOf(what)) !== -1) {
            arr.splice(ax, 1);
        }
    }
    return arr;
}
