using ClientMinimalApi.Data;
using ClientMinimalApi.Data.Entities;
using ClientMinimalApi.Dto;
using Microsoft.EntityFrameworkCore;

namespace ClientMinimalApi.Services
{
    public interface IDeviceService
    {
        Task<bool> CheckPasswordByDeviceId(PasswordRequest request, CancellationToken cancellationToken);
        Task RegisterAlarm(AlarmRequest request, CancellationToken cancellationToken);
        Task<IEnumerable<Card>> GetCards(CancellationToken cancellationToken);
        Task<IEnumerable<Setting>> GetSettings(CancellationToken cancellationToken);
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
                var devicePassword = await dbContext.Devices
                        .Where(d => d.Password == request.Password)
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
                var deviceId = await dbContext.Devices
                    .Where(d => d.DeviceId == request.DeviceId)
                    .Select(x => x.Id)
                    .FirstOrDefaultAsync(cancellationToken);

                if (deviceId is 0)
                {
                    throw new InvalidOperationException($"Device with id: {request.DeviceId} was not found");
                }

                var alarmEntiry = new Alarm
                {
                    Data = request.Message,
                    DeviceId = deviceId,
                    Date = DateTimeOffset.Now.Date
                };
                
                dbContext.Alarm.Add(alarmEntiry);
                await dbContext.SaveChangesAsync(cancellationToken);
            }
            catch (Exception ex) 
            {
                throw new Exception(ex.Message);
            }
        }

        public async Task<IEnumerable<Card>> GetCards(CancellationToken cancellationToken)
        {
            try
            {
                var cards = await dbContext.Cards
                    .ToListAsync(cancellationToken);

                return cards;
            }
            catch (Exception ex)
            {
                throw new Exception(ex.Message);
            }
        }

        public async Task<IEnumerable<Setting>> GetSettings(CancellationToken cancellationToken)
        {
            try
            {
                var settings = await dbContext.Settings
                    .ToListAsync(cancellationToken);

                return settings;
            }
            catch (Exception ex)
            {
                throw new Exception(ex.Message);
            }
        }
    }
}