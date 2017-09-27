var _borders = ["rgb(255, 99, 132)", "rgb(255, 159, 64)", "rgb(255, 205, 86)", "rgb(75, 192, 192)", "rgb(54, 162, 235)", "rgb(153, 102, 255)", "rgb(201, 203, 207)"];
var _colors = ["rgba(255, 99, 132, 0.5)", "rgba(255, 159, 64, 0.5)", "rgba(255, 205, 86, 0.5)", "rgba(75, 192, 192, 0.5)", "rgba(54, 162, 235, 0.5)", "rgba(153, 102, 255, 0.5)", "rgba(201, 203, 207, 0.5)"];

function callAjax(url, callback, callbackparam){
    var xmlhttp;
    xmlhttp = new XMLHttpRequest();
    xmlhttp.onreadystatechange = function(){
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200 && callback){
	        if (!callbackparam)
	            callback(xmlhttp.responseText);
	        else
		        callback(xmlhttp.responseText, callbackparam);
        }
    }
    xmlhttp.open("GET", url, true);
    xmlhttp.send();
}
callAjax("/genders.txt", chart, {id: "genres", type: "bar", legend: false, grid: true, max_size: -1});
callAjax("/nationalities.txt", chart, {id: "nationalities", type: "doughnut", legend: true, grid: false, max_size: 10});
callAjax("/status.txt", chart, {id: "status", type: "doughnut", legend: true, grid: false, max_size: -1});
callAjax("/ngramslength.txt", line, {id: "ngramslength", type: "line", legend: true, grid: true, max_size: -1});

var density;
function chart(txt, params) {
	txt = txt.split("\n");
	density = {};
	txt.forEach(function(line) {
		if (!line) return;
		if (!density[line.trim()]) density[line.trim()] = 0;
		density[line.trim()]++;
	});
	var keys = Object.keys(density).sort(function(a, b) {
	  return density[b] - density[a];
	});
	var values = Object.values(density).sort(function(a, b) {
	  return b - a;
	});
	if (params.max_size > 0) {
		var keys2 = [], values2 = [];
		for (var i = 0; i < params.max_size; i++) {
			keys2.push(keys[i]);
			values2.push(values[i]);
		}
		keys2.push("Autres");
		var sum = 0;
		for (var i = params.max_size; i < values.length; i++) sum += values[i];
		values2.push(sum);
		keys = keys2;
		values = values2;
	}
	
	
	var colors = [], borders = [];
	for (var i = 0; i < keys.length; i++) {
		colors[i] = _colors[i%_colors.length];
		borders[i] = _borders[i%_borders.length];
	}
	var options = {
		maintainAspectRatio: true,
		spanGaps: false,
		elements: {
			line: {
				tension: 0.000001
			}
		},
		plugins: {
			filler: {
				propagate: false
			}
		},
		legend: {
	        display: params.legend
	    }
	}
	if (params.grid) {
		options.scales = {
			yAxes: [{
				ticks: {
					beginAtZero: true
				}
			}],
			xAxes: [{
				ticks: {
					autoSkip: false
				}
			}]
		}
	}
	var chart = new Chart(params.id, {
		type: params.type,
		data: data = {
			labels: keys,
			datasets: [{
				backgroundColor: colors,
				borderColor: borders,
				data: values,
				label: 'Somme '+params.id,
			}]
		},
		options: options
	});
}



function line(txt, params) {
	txt = txt.split("\n");
	density = [];
	txt.forEach(function(line) {
		if (!line) return;
		if (!density[line.trim()]) density[line.trim()] = 0;
		density[line.trim()]++;
	});
	density = density.sort(function(a, b) {
	   return a < b;
	});
	var options = {
		maintainAspectRatio: true,
		spanGaps: false,
		elements: {
			line: {
				tension: 0.000001
			}
		},
		plugins: {
			filler: {
				propagate: false
			}
		},
		legend: {
	        display: params.legend
	    }
	}
	var chart = new Chart(params.id, {
		type: params.type,
		data: data = {
			labels: Object.keys(density),
			datasets: [{
				backgroundColor: _colors[4],
				borderColor: _borders[4],
				data: Object.values(density),
				label: 'Somme '+params.id,
			}]
		},
		options: options
	});
}


window.onload = function() {
	var ctx = document.getElementById("genders");
	var genders;
}
