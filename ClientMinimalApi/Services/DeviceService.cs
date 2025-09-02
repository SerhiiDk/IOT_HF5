using ClientMinimalApi.Data;
using ClientMinimalApi.Data.Entities;
using ClientMinimalApi.Dto;
using Microsoft.AspNetCore.Http.HttpResults;
using Microsoft.EntityFrameworkCore;
using System.Diagnostics;

namespace ClientMinimalApi.Services
{

    public interface IDeviceService
    {
        Task<bool> CheckPasswordByDeviceId(PasswordRequest request, CancellationToken cancellationToken);
        Task RegisterAlarm(AlarmRequest request, CancellationToken cancellationToken);
        Task<IEnumerable<Card>> GetCards(CancellationToken cancellationToken);
    }

    public class DeviceService : IDeviceService
    {
        private readonly DataContext dbContext;

        public DeviceService(DataContext dbContext)
        {
            this.dbContext = dbContext;

        }

        public async Task<bool> CheckPasswordByDeviceId(PasswordRequest request, CancellationToken cancellationToken)
        {
            try
            {
                var devicePassword = await dbContext.Set<Devices>()
                        .Where(d => d.DeviceId == request.DeviceId)
                        .Select(d => d.Password)
                        .FirstOrDefaultAsync(cancellationToken);

                return devicePassword == request.Password;

            }
            catch (Exception ex)
            {
                throw new Exception("Problem on server");
            }
        }

        public async Task RegisterAlarm(AlarmRequest request, CancellationToken cancellationToken)
        {
            try
            {
                var deviceId = await dbContext.Set<Devices>()
                    .Where(d => d.DeviceId == request.DeviceId)
                    .Select(x => x.Id)
                    .FirstOrDefaultAsync(cancellationToken);

                if (deviceId is 0)
                {
                    throw new InvalidOperationException($"Device with id: {request.DeviceId} was not found");
                }

                var test = new DateTimeOffset().Date;
                var alarmEntiry = new Alarm
                {
                    Data = request.Message,
                    DeviceId = deviceId,
                    Date = new DateTimeOffset().Date
                };
            }
            catch (Exception Ex) 
            {
                throw;
            }
        }

        public async Task<IEnumerable<Card>> GetCards(CancellationToken cancellationToken)
        {
            var cards = await dbContext.Set<Card>()
                .ToListAsync();

            return cards;
        }
    }
}