using ClientMinimalApi.Data;
using ClientMinimalApi.Dto;
using ClientMinimalApi.Services;
using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using MQTTnet;
using System.Text;
using System.Text.Json;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddScoped<IDeviceService, DeviceService>();
// create mqtt client
builder.Services.AddSingleton<IMqttClient>(sp =>
{
    var factory = new MqttClientFactory();
    return factory.CreateMqttClient();
});

// allow other devices to connect to API
builder.WebHost.ConfigureKestrel(serverOptions =>
{
    serverOptions.Listen(System.Net.IPAddress.Any, 8000); 
});

// create mqtt options (broker name, connection ip, port)
builder.Services.AddSingleton<MqttClientOptions>(opt =>
{
    return new MqttClientOptionsBuilder()
        .WithClientId("Rasperi4")
        .WithTcpServer("192.168.1.124", 1883)
        .WithCleanSession()
        .Build();
});

// TODO: User .net manager(for key)
string connection = "server=192.168.1.124;database=IOT;user=Medarbejder;password=chrser123!;";

// add connection to database  
builder.Services.AddDbContext<DataContext>(options =>
{
    options.UseMySql(connection, new MySqlServerVersion(new Version(10, 6)))
        .LogTo(Console.WriteLine, LogLevel.Information)
        .EnableSensitiveDataLogging()
        .EnableDetailedErrors();
});

    
// swagger
builder.Services.AddEndpointsApiExplorer();
builder.Services.AddSwaggerGen();

var app = builder.Build();

// mqtt
var mqttClient = app.Services.GetRequiredService<IMqttClient>();
var mqttOptions = app.Services.GetRequiredService<MqttClientOptions>();

var serviceProvider = app.Services;

// mqtt handler
mqttClient.ApplicationMessageReceivedAsync += HandleReceivedApplicationMessage;
mqttClient.DisconnectedAsync += e =>
{
    return Task.CompletedTask;
};

// setup mqttp client
await mqttClient.ConnectAsync(mqttOptions, CancellationToken.None);
await mqttClient.SubscribeAsync("Alarm");

// Configure the HTTP request pipeline.
if (app.Environment.IsDevelopment())
{
    app.UseSwagger();
    app.UseSwaggerUI();
}

async Task HandleReceivedApplicationMessage(MqttApplicationMessageReceivedEventArgs e)
{
    var test = Convert.ToString(e.ApplicationMessage.Payload);
    if (e.ApplicationMessage.Topic.Equals("Alarm"))
    {
        try
        {
            var data = System.Text.Encoding.UTF8.GetString(e.ApplicationMessage.Payload).Split(",");

            using var scope = serviceProvider.CreateScope();
            var service = scope.ServiceProvider.GetRequiredService<IDeviceService>();

            await service.RegisterAlarm(new AlarmRequest(data[0], data[2]), CancellationToken.None);
        }
        catch (Exception ex)
        {
            throw new Exception(ex.Message);
        }
    }
    
    await Task.CompletedTask;
}

// returns 200//401
app.MapPost("/verifyPassword", async ([FromServices] IDeviceService service,  HttpRequest request, CancellationToken cancellationToken) =>
{
    try
    {
        using var reader = new StreamReader(request.Body);
        var rawJson = await reader.ReadToEndAsync(cancellationToken);

        var values = JsonSerializer.Deserialize<string[]>(rawJson);

        if(values is null || string.IsNullOrEmpty(values[0]) || string.IsNullOrEmpty(values[1]))
        {
            return Results.Problem();
        }

        bool isValid = await service.CheckPasswordByDeviceId(new PasswordRequest(values[0], values[1]), cancellationToken);

        return isValid
            ? Results.Ok()
            : Results.Unauthorized();
    }
    catch (Exception ex)
    {
        return Results.Problem();
    }
});

// return array of cards
app.MapGet("/getCards", async ([FromServices] IDeviceService service, CancellationToken cancellation = default) =>
{
    try
    {
        var cards = await service.GetCards(cancellation);
        return Results.Ok(cards);
    }
    catch (Exception ex)
    {
        return Results.Problem();
    }
});

// return array of settings
app.MapGet("/getSettings", async ([FromServices] IDeviceService service, CancellationToken cancellation = default) =>
{
    try
    {
        var settings = await service.GetSettings(cancellation);
        return Results.Ok(settings);
    }
    catch (Exception ex)
    {
        return Results.Problem();
    }
});

app.Run();